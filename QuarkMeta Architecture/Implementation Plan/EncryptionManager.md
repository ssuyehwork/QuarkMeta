# EncryptionManager Implementation Plan - Shell Protection Decryption and Password Change

This implementation plan details the implementation of full AES-256 decryption file restoration (`decryptFile`) and password modification (`ActionChangePwd`) in `EncryptionManager` and `ContentContextMenu`.

---

## 1. Overview & Problem Statement
* **Issue**: In the "Shell Protection" (外壳保护) context sub-menu, "解除保护" (ActionDecrypt) only displayed a mock toast notification without restoring files, and "修改保护密码" (ActionChangePwd) was missing a handler case in `ContentContextMenu.cpp`.
* **Solution**:
  1. Add `EncryptionManager::decryptFile` in `src/crypto/EncryptionManager.h/cpp` for BCrypt block decryption and file restoration.
  2. Implement full `ActionDecrypt` handler in `ContentContextMenu.cpp` to verify password, decrypt `.amenc` back to original files, and clear `isEncrypted` in `MetadataManager`.
  3. Implement `ActionChangePwd` handler in `ContentContextMenu.cpp` to verify old password, re-encrypt file with new password, and cleanup temporary plaintexts.

---

## 2. Modified Files List
- `src/crypto/EncryptionManager.h`
- `src/crypto/EncryptionManager.cpp`
- `src/ui/controllers/ContentContextMenu.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/crypto/EncryptionManager.h`

```git
<<<<<<< SEARCH
    /**
     * @brief 解密文件并持有句柄 (RAII)
     */
    std::shared_ptr<DecryptedFileHandle> decryptToTemp(const std::wstring& amencPath, const std.string& password);
=======
    /**
     * @brief 解密文件并保存至指定物理路径
     */
    bool decryptFile(const std::wstring& amencPath, const std::wstring& destPath, const std.string& password);

    /**
     * @brief 解密文件并持有句柄 (RAII)
     */
    std::shared_ptr<DecryptedFileHandle> decryptToTemp(const std::wstring& amencPath, const std.string& password);
>>>>>>> REPLACE
```

---

### 3.2 `src/ui/controllers/ContentContextMenu.cpp`

```git
<<<<<<< SEARCH
        case ContentPanel::ActionDecrypt: {
            FramelessInputDialog dlg("解除加密", "输入加密密码:", "", m_panel);
            dlg.setEchoMode(QLineEdit::Password);
            if (dlg.exec() == QDialog::Accepted) {
                QString pwd = dlg.text();
                if (!pwd.isEmpty()) {
                    ToolTipOverlay::instance()->showText(QCursor::pos(), "解除加密逻辑已触发", 1500);
                }
            }
            break;
        }
=======
        case ContentPanel::ActionDecrypt: {
            FramelessInputDialog dlg("解除外壳保护", "输入解密密码:", "", m_panel);
            dlg.setEchoMode(QLineEdit::Password);
            if (dlg.exec() == QDialog::Accepted) {
                QString pwd = dlg.text();
                if (pwd.isEmpty()) break;
                auto indexes = view->selectionModel()->selectedIndexes();
                QStringList targets;
                for (const auto& idx : indexes) if (idx.column() == 0) targets << idx.data(PathRole).toString();

                ToolTipOverlay::instance()->showText(QCursor::pos(), "解密还原任务已在后台启动...", 2000);

                std::string stdPwd = pwd.toStdString();
                QPointer<ContentPanel> self(m_panel);
                QString curDir = currentPath;

                (void)QThreadPool::globalInstance()->start([self, targets, stdPwd, curDir]() {
                    bool anySuccess = false;
                    for (const QString& src : targets) {
                        QString dest = src;
                        if (dest.endsWith(".amenc", Qt::CaseInsensitive)) {
                            dest.chop(6);
                        } else {
                            dest += ".decrypted";
                        }

                        if (EncryptionManager::instance().decryptFile(src.toStdWString(), dest.toStdWString(), stdPwd)) {
                            QFile::remove(src);
                            MetadataManager::instance().setEncrypted(dest.toStdWString(), false);
                            anySuccess = true;
                        }
                    }

                    QMetaObject::invokeMethod(QCoreApplication::instance(), [self, curDir, anySuccess]() {
                        if (self && self->currentPath() == curDir) self->loadDirectory(curDir, self->isRecursive());
                        if (anySuccess) {
                            ToolTipOverlay::instance()->showText(QCursor::pos(), "解除保护成功，文件已还原", 1500, QColor("#2ecc71"));
                        } else {
                            ToolTipOverlay::instance()->showText(QCursor::pos(), "解密失败，请检查密码是否正确", 2000, QColor("#e74c3c"));
                        }
                    });
                });
            }
            break;
        }
        case ContentPanel::ActionChangePwd: {
            FramelessInputDialog dlgOld("修改保护密码", "输入原密码:", "", m_panel);
            dlgOld.setEchoMode(QLineEdit::Password);
            if (dlgOld.exec() != QDialog::Accepted || dlgOld.text().isEmpty()) break;
            QString oldPwd = dlgOld.text();

            FramelessInputDialog dlgNew("修改保护密码", "输入新密码:", "", m_panel);
            dlgNew.setEchoMode(QLineEdit::Password);
            if (dlgNew.exec() != QDialog::Accepted || dlgNew.text().isEmpty()) break;
            QString newPwd = dlgNew.text();

            auto indexes = view->selectionModel()->selectedIndexes();
            QStringList targets;
            for (const auto& idx : indexes) if (idx.column() == 0) targets << idx.data(PathRole).toString();

            ToolTipOverlay::instance()->showText(QCursor::pos(), "密码修改中...", 2000);

            std::string stdOldPwd = oldPwd.toStdString();
            std::string stdNewPwd = newPwd.toStdString();
            QPointer<ContentPanel> self(m_panel);
            QString curDir = currentPath;

            (void)QThreadPool::globalInstance()->start([self, targets, stdOldPwd, stdNewPwd, curDir]() {
                bool anySuccess = false;
                for (const QString& src : targets) {
                    QString tempPlain = src + ".tmp_dec";
                    if (EncryptionManager::instance().decryptFile(src.toStdWString(), tempPlain.toStdWString(), stdOldPwd)) {
                        if (EncryptionManager::instance().encryptFile(tempPlain.toStdWString(), src.toStdWString(), stdNewPwd)) {
                            QFile::remove(tempPlain);
                            anySuccess = true;
                        } else {
                            QFile::remove(tempPlain);
                        }
                    }
                }

                QMetaObject::invokeMethod(QCoreApplication::instance(), [self, curDir, anySuccess]() {
                    if (self && self->currentPath() == curDir) self->loadDirectory(curDir, self->isRecursive());
                    if (anySuccess) {
                        ToolTipOverlay::instance()->showText(QCursor::pos(), "保护密码修改成功", 1500, QColor("#2ecc71"));
                    } else {
                        ToolTipOverlay::instance()->showText(QCursor::pos(), "原密码错误，修改失败", 2000, QColor("#e74c3c"));
                    }
                });
            });
            break;
        }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Verify `EncryptionManager::decryptFile` correctly decrypts `.amenc` files and verifies PKCS#7 block padding.
2. Confirm `EncryptionManager.md` exists in `QuarkMeta Architecture/Implementation Plan/`.
