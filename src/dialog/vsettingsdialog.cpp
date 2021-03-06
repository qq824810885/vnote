#include "vsettingsdialog.h"
#include <QtWidgets>
#include <QRegExp>
#include "vconfigmanager.h"
#include "utils/vutils.h"
#include "vconstants.h"

extern VConfigManager *g_config;

VSettingsDialog::VSettingsDialog(QWidget *p_parent)
    : QDialog(p_parent)
{
    m_tabList = new QListWidget(this);
    m_tabList->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    m_tabs = new QStackedLayout();

    m_btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_btnBox, &QDialogButtonBox::accepted, this, &VSettingsDialog::saveConfiguration);
    connect(m_btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QHBoxLayout *tabLayout = new QHBoxLayout();
    tabLayout->addWidget(m_tabList);
    tabLayout->addLayout(m_tabs);
    tabLayout->setContentsMargins(0, 0, 0, 0);
    tabLayout->setSpacing(0);
    tabLayout->setStretch(0, 0);
    tabLayout->setStretch(1, 5);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(tabLayout);
    mainLayout->addWidget(m_btnBox);
    setLayout(mainLayout);

    setWindowTitle(tr("Settings"));

    // Add tabs.
    addTab(new VGeneralTab(), tr("General"));
    addTab(new VReadEditTab(), tr("Read/Edit"));
    addTab(new VNoteManagementTab(), tr("Note Management"));
    addTab(new VMarkdownTab(), tr("Markdown"));

    m_tabList->setMaximumWidth(m_tabList->sizeHintForColumn(0) + 5);

    connect(m_tabList, &QListWidget::currentItemChanged,
            this, [this](QListWidgetItem *p_cur, QListWidgetItem *p_pre) {
                Q_UNUSED(p_pre);
                Q_ASSERT(p_cur);
                int idx = p_cur->data(Qt::UserRole).toInt();
                Q_ASSERT(idx >= 0);
                m_tabs->setCurrentWidget(m_tabs->widget(idx));
            });

    m_tabList->setCurrentRow(0);

    loadConfiguration();
}

void VSettingsDialog::addTab(QWidget *p_widget, const QString &p_label)
{
    int idx = m_tabs->addWidget(p_widget);
    QListWidgetItem *item = new QListWidgetItem(p_label, m_tabList);
    item->setData(Qt::UserRole, idx);
}

void VSettingsDialog::loadConfiguration()
{
    // General Tab.
    {
        VGeneralTab *generalTab = dynamic_cast<VGeneralTab *>(m_tabs->widget(0));
        Q_ASSERT(generalTab);
        if (!generalTab->loadConfiguration()) {
            goto err;
        }
    }

    // Read/Edit Tab.
    {
        VReadEditTab *readEditTab = dynamic_cast<VReadEditTab *>(m_tabs->widget(1));
        Q_ASSERT(readEditTab);
        if (!readEditTab->loadConfiguration()) {
            goto err;
        }
    }

    // Note Management Tab.
    {
        VNoteManagementTab *noteManagementTab = dynamic_cast<VNoteManagementTab *>(m_tabs->widget(2));
        Q_ASSERT(noteManagementTab);
        if (!noteManagementTab->loadConfiguration()) {
            goto err;
        }
    }

    // Markdown Tab.
    {
        VMarkdownTab *markdownTab = dynamic_cast<VMarkdownTab *>(m_tabs->widget(3));
        Q_ASSERT(markdownTab);
        if (!markdownTab->loadConfiguration()) {
            goto err;
        }
    }

    return;
err:
    VUtils::showMessage(QMessageBox::Warning, tr("Warning"),
                        tr("Fail to load configuration."), "",
                        QMessageBox::Ok, QMessageBox::Ok, NULL);
    QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
}

void VSettingsDialog::saveConfiguration()
{
    // General Tab.
    {
        VGeneralTab *generalTab = dynamic_cast<VGeneralTab *>(m_tabs->widget(0));
        Q_ASSERT(generalTab);
        if (!generalTab->saveConfiguration()) {
            goto err;
        }
    }

    // Read/Edit Tab.
    {
        VReadEditTab *readEditTab = dynamic_cast<VReadEditTab *>(m_tabs->widget(1));
        Q_ASSERT(readEditTab);
        if (!readEditTab->saveConfiguration()) {
            goto err;
        }
    }

    // Note Management Tab.
    {
        VNoteManagementTab *noteManagementTab = dynamic_cast<VNoteManagementTab *>(m_tabs->widget(2));
        Q_ASSERT(noteManagementTab);
        if (!noteManagementTab->saveConfiguration()) {
            goto err;
        }
    }

    // Markdown Tab.
    {
        VMarkdownTab *markdownTab = dynamic_cast<VMarkdownTab *>(m_tabs->widget(3));
        Q_ASSERT(markdownTab);
        if (!markdownTab->saveConfiguration()) {
            goto err;
        }
    }

    accept();
    return;
err:
    VUtils::showMessage(QMessageBox::Warning, tr("Warning"),
                        tr("Fail to save configuration. Please try it again."), "",
                        QMessageBox::Ok, QMessageBox::Ok, NULL);
}

const QVector<QString> VGeneralTab::c_availableLangs = { "System", "English", "Chinese" };

VGeneralTab::VGeneralTab(QWidget *p_parent)
    : QWidget(p_parent)
{
    // Language combo.
    m_langCombo = VUtils::getComboBox(this);
    m_langCombo->setToolTip(tr("Choose the language of VNote interface"));
    m_langCombo->addItem(tr("System"), "System");
    auto langs = VUtils::getAvailableLanguages();
    for (auto const &lang : langs) {
        m_langCombo->addItem(lang.second, lang.first);
    }

    // System tray checkbox.
    m_systemTray = new QCheckBox(tr("System tray"), this);
    m_systemTray->setToolTip(tr("Minimized to the system tray after closing VNote"
                                " (not supported in macOS)"));
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    // Do not support minimized to tray on macOS.
    m_systemTray->setEnabled(false);
#endif

    // Startup pages.
    QLayout *startupLayout = setupStartupPagesLayout();

    QFormLayout *optionLayout = new QFormLayout();
    optionLayout->addRow(tr("Language:"), m_langCombo);
    optionLayout->addRow(m_systemTray);
    optionLayout->addRow(tr("Startup pages:"), startupLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(optionLayout);

    setLayout(mainLayout);
}

QLayout *VGeneralTab::setupStartupPagesLayout()
{
    m_startupPageTypeCombo = VUtils::getComboBox(this);
    m_startupPageTypeCombo->setToolTip(tr("Restore tabs or open specific notes on startup"));
    m_startupPageTypeCombo->addItem(tr("None"), (int)StartupPageType::None);
    m_startupPageTypeCombo->addItem(tr("Continue where you left off"), (int)StartupPageType::ContinueLeftOff);
    m_startupPageTypeCombo->addItem(tr("Open specific pages"), (int)StartupPageType::SpecificPages);
    connect(m_startupPageTypeCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, [this](int p_index) {
                int type = m_startupPageTypeCombo->itemData(p_index).toInt();
                bool pagesEditVisible = type == (int)StartupPageType::SpecificPages;
                m_startupPagesEdit->setVisible(pagesEditVisible);
                m_startupPagesAddBtn->setVisible(pagesEditVisible);
            });

    m_startupPagesEdit = new QPlainTextEdit(this);
    m_startupPagesEdit->setToolTip(tr("Absolute path of the notes to open on startup (one note per line)"));

    m_startupPagesAddBtn = new QPushButton(tr("Browse"), this);
    m_startupPagesAddBtn->setToolTip(tr("Select files to add as startup pages"));
    connect(m_startupPagesAddBtn, &QPushButton::clicked,
            this, [this]() {
                static QString lastPath = QDir::homePath();
                QStringList files = QFileDialog::getOpenFileNames(this,
                                                                  tr("Select Files As Startup Pages"),
                                                                  lastPath);
                if (files.isEmpty()) {
                    return;
                }

                // Update lastPath
                lastPath = QFileInfo(files[0]).path();

                m_startupPagesEdit->appendPlainText(files.join("\n"));
            });

    QHBoxLayout *startupPagesBtnLayout = new QHBoxLayout();
    startupPagesBtnLayout->addStretch();
    startupPagesBtnLayout->addWidget(m_startupPagesAddBtn);

    QVBoxLayout *startupPagesLayout = new QVBoxLayout();
    startupPagesLayout->addWidget(m_startupPagesEdit);
    startupPagesLayout->addLayout(startupPagesBtnLayout);

    QVBoxLayout *startupLayout = new QVBoxLayout();
    startupLayout->addWidget(m_startupPageTypeCombo);
    startupLayout->addLayout(startupPagesLayout);

    m_startupPagesEdit->hide();
    m_startupPagesAddBtn->hide();

    return startupLayout;
}

bool VGeneralTab::loadConfiguration()
{
    if (!loadLanguage()) {
        return false;
    }

    if (!loadSystemTray()) {
        return false;
    }

    if (!loadStartupPageType()) {
        return false;
    }

    return true;
}

bool VGeneralTab::saveConfiguration()
{
    if (!saveLanguage()) {
        return false;
    }

    if (!saveSystemTray()) {
        return false;
    }

    if (!saveStartupPageType()) {
        return false;
    }

    return true;
}

bool VGeneralTab::loadLanguage()
{
    QString lang = g_config->getLanguage();
    if (lang.isNull()) {
        return false;
    } else if (lang == "System") {
        m_langCombo->setCurrentIndex(0);
        return true;
    }
    bool found = false;
    // lang is the value, not name.
    for (int i = 0; i < m_langCombo->count(); ++i) {
        if (m_langCombo->itemData(i).toString() == lang) {
            found = true;
            m_langCombo->setCurrentIndex(i);
            break;
        }
    }
    if (!found) {
        qWarning() << "invalid language configuration (using default value)";
        m_langCombo->setCurrentIndex(0);
    }
    return true;
}

bool VGeneralTab::saveLanguage()
{
    QString curLang = m_langCombo->currentData().toString();
    g_config->setLanguage(curLang);
    return true;
}

bool VGeneralTab::loadSystemTray()
{
    m_systemTray->setChecked(g_config->getMinimizeToStystemTray() != 0);
    return true;
}

bool VGeneralTab::saveSystemTray()
{
    if (m_systemTray->isEnabled()) {
        g_config->setMinimizeToSystemTray(m_systemTray->isChecked() ? 1 : 0);
    }

    return true;
}

bool VGeneralTab::loadStartupPageType()
{
    StartupPageType type = g_config->getStartupPageType();
    bool found = false;
    for (int i = 0; i < m_startupPageTypeCombo->count(); ++i) {
        if (m_startupPageTypeCombo->itemData(i).toInt() == (int)type) {
            found = true;
            m_startupPageTypeCombo->setCurrentIndex(i);
        }
    }

    Q_ASSERT(found);

    const QStringList &pages = g_config->getStartupPages();
    m_startupPagesEdit->setPlainText(pages.join("\n"));

    bool pagesEditVisible = type == StartupPageType::SpecificPages;
    m_startupPagesEdit->setVisible(pagesEditVisible);
    m_startupPagesAddBtn->setVisible(pagesEditVisible);

    return true;
}

bool VGeneralTab::saveStartupPageType()
{
    StartupPageType type = (StartupPageType)m_startupPageTypeCombo->currentData().toInt();
    g_config->setStartupPageType(type);

    if (type == StartupPageType::SpecificPages) {
        QStringList pages = m_startupPagesEdit->toPlainText().split("\n");
        g_config->setStartupPages(pages);
    }

    return true;
}

VReadEditTab::VReadEditTab(QWidget *p_parent)
    : QWidget(p_parent)
{
    m_readBox = new QGroupBox(tr("Read Mode (For Markdown Only)"));
    m_editBox = new QGroupBox(tr("Edit Mode"));

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(m_readBox);
    mainLayout->addWidget(m_editBox);
    setLayout(mainLayout);
}

bool VReadEditTab::loadConfiguration()
{
    return true;
}

bool VReadEditTab::saveConfiguration()
{
    return true;
}

VNoteManagementTab::VNoteManagementTab(QWidget *p_parent)
    : QWidget(p_parent)
{
    m_noteBox = new QGroupBox(tr("Notes"));
    m_externalBox = new QGroupBox(tr("External Files"));

    // Note.
    // Image folder.
    m_customImageFolder = new QCheckBox(tr("Custom image folder"), this);
    m_customImageFolder->setToolTip(tr("Set the global name of the image folder to hold images "
                                       "of notes (restart VNote to make it work)"));
    connect(m_customImageFolder, &QCheckBox::stateChanged,
            this, &VNoteManagementTab::customImageFolderChanged);

    m_imageFolderEdit = new QLineEdit(this);
    m_imageFolderEdit->setPlaceholderText(tr("Name of the image folder"));
    m_imageFolderEdit->setToolTip(m_customImageFolder->toolTip());
    QValidator *validator = new QRegExpValidator(QRegExp(VUtils::c_fileNameRegExp), this);
    m_imageFolderEdit->setValidator(validator);

    QHBoxLayout *imageFolderLayout = new QHBoxLayout();
    imageFolderLayout->addWidget(m_customImageFolder);
    imageFolderLayout->addWidget(m_imageFolderEdit);

    // Attachment folder.
    m_customAttachmentFolder = new QCheckBox(tr("Custom attachment folder"), this);
    m_customAttachmentFolder->setToolTip(tr("Set the global name of the attachment folder to hold attachments "
                                            "of notes (restart VNote to make it work)"));
    connect(m_customAttachmentFolder, &QCheckBox::stateChanged,
            this, &VNoteManagementTab::customAttachmentFolderChanged);

    m_attachmentFolderEdit = new QLineEdit(this);
    m_attachmentFolderEdit->setPlaceholderText(tr("Name of the attachment folder"));
    m_attachmentFolderEdit->setToolTip(m_customAttachmentFolder->toolTip());
    validator = new QRegExpValidator(QRegExp(VUtils::c_fileNameRegExp), this);
    m_attachmentFolderEdit->setValidator(validator);

    QHBoxLayout *attachmentFolderLayout = new QHBoxLayout();
    attachmentFolderLayout->addWidget(m_customAttachmentFolder);
    attachmentFolderLayout->addWidget(m_attachmentFolderEdit);

    QFormLayout *noteLayout = new QFormLayout();
    noteLayout->addRow(imageFolderLayout);
    noteLayout->addRow(attachmentFolderLayout);
    m_noteBox->setLayout(noteLayout);

    // External File.
    // Image folder.
    m_customImageFolderExt = new QCheckBox(tr("Custom image folder"), this);
    m_customImageFolderExt->setToolTip(tr("Set the path of the global image folder to hold images "
                                          "of external files (restart VNote to make it work).\nYou "
                                          "could use both absolute or relative path here. If "
                                          "absolute path is used, VNote will not manage\nthose images, "
                                          "so you need to clean up unused images manually."));
    connect(m_customImageFolderExt, &QCheckBox::stateChanged,
            this, &VNoteManagementTab::customImageFolderExtChanged);

    m_imageFolderEditExt = new QLineEdit(this);
    m_imageFolderEditExt->setToolTip(m_customImageFolderExt->toolTip());
    m_imageFolderEditExt->setPlaceholderText(tr("Name of the image folder"));

    QHBoxLayout *imageFolderExtLayout = new QHBoxLayout();
    imageFolderExtLayout->addWidget(m_customImageFolderExt);
    imageFolderExtLayout->addWidget(m_imageFolderEditExt);

    QFormLayout *externalLayout = new QFormLayout();
    externalLayout->addRow(imageFolderExtLayout);
    m_externalBox->setLayout(externalLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(m_noteBox);
    mainLayout->addWidget(m_externalBox);

    setLayout(mainLayout);
}

bool VNoteManagementTab::loadConfiguration()
{
    if (!loadImageFolder()) {
        return false;
    }

    if (!loadAttachmentFolder()) {
        return false;
    }

    if (!loadImageFolderExt()) {
        return false;
    }

    return true;
}

bool VNoteManagementTab::saveConfiguration()
{
    if (!saveImageFolder()) {
        return false;
    }

    if (!saveAttachmentFolder()) {
        return false;
    }

    if (!saveImageFolderExt()) {
        return false;
    }

    return true;
}

bool VNoteManagementTab::loadImageFolder()
{
    bool isCustom = g_config->isCustomImageFolder();

    m_customImageFolder->setChecked(isCustom);
    m_imageFolderEdit->setText(g_config->getImageFolder());
    m_imageFolderEdit->setEnabled(isCustom);

    return true;
}

bool VNoteManagementTab::saveImageFolder()
{
    if (m_customImageFolder->isChecked()) {
        g_config->setImageFolder(m_imageFolderEdit->text());
    } else {
        g_config->setImageFolder("");
    }

    return true;
}

void VNoteManagementTab::customImageFolderChanged(int p_state)
{
    if (p_state == Qt::Checked) {
        m_imageFolderEdit->setEnabled(true);
        m_imageFolderEdit->selectAll();
        m_imageFolderEdit->setFocus();
    } else {
        m_imageFolderEdit->setEnabled(false);
    }
}

bool VNoteManagementTab::loadAttachmentFolder()
{
    bool isCustom = g_config->isCustomAttachmentFolder();

    m_customAttachmentFolder->setChecked(isCustom);
    m_attachmentFolderEdit->setText(g_config->getAttachmentFolder());
    m_attachmentFolderEdit->setEnabled(isCustom);

    return true;
}

bool VNoteManagementTab::saveAttachmentFolder()
{
    if (m_customAttachmentFolder->isChecked()) {
        g_config->setAttachmentFolder(m_attachmentFolderEdit->text());
    } else {
        g_config->setAttachmentFolder("");
    }

    return true;
}

void VNoteManagementTab::customAttachmentFolderChanged(int p_state)
{
    if (p_state == Qt::Checked) {
        m_attachmentFolderEdit->setEnabled(true);
        m_attachmentFolderEdit->selectAll();
        m_attachmentFolderEdit->setFocus();
    } else {
        m_attachmentFolderEdit->setEnabled(false);
    }
}

bool VNoteManagementTab::loadImageFolderExt()
{
    bool isCustom = g_config->isCustomImageFolderExt();

    m_customImageFolderExt->setChecked(isCustom);
    m_imageFolderEditExt->setText(g_config->getImageFolderExt());
    m_imageFolderEditExt->setEnabled(isCustom);

    return true;
}

bool VNoteManagementTab::saveImageFolderExt()
{
    if (m_customImageFolderExt->isChecked()) {
        g_config->setImageFolderExt(m_imageFolderEditExt->text());
    } else {
        g_config->setImageFolderExt("");
    }

    return true;
}

void VNoteManagementTab::customImageFolderExtChanged(int p_state)
{
    if (p_state == Qt::Checked) {
        m_imageFolderEditExt->setEnabled(true);
        m_imageFolderEditExt->selectAll();
        m_imageFolderEditExt->setFocus();
    } else {
        m_imageFolderEditExt->setEnabled(false);
    }
}

VMarkdownTab::VMarkdownTab(QWidget *p_parent)
    : QWidget(p_parent)
{
    // Default note open mode.
    m_openModeCombo = VUtils::getComboBox();
    m_openModeCombo->setToolTip(tr("Default mode to open a note"));
    m_openModeCombo->addItem(tr("Read Mode"), (int)OpenFileMode::Read);
    m_openModeCombo->addItem(tr("Edit Mode"), (int)OpenFileMode::Edit);

    // Heading sequence.
    m_headingSequenceTypeCombo = VUtils::getComboBox();
    m_headingSequenceTypeCombo->setToolTip(tr("Enable auto sequence for all headings (in the form like 1.2.3.4.)"));
    m_headingSequenceTypeCombo->addItem(tr("Disabled"), (int)HeadingSequenceType::Disabled);
    m_headingSequenceTypeCombo->addItem(tr("Enabled"), (int)HeadingSequenceType::Enabled);
    m_headingSequenceTypeCombo->addItem(tr("Enabled for notes only"), (int)HeadingSequenceType::EnabledNoteOnly);

    m_headingSequenceLevelCombo = VUtils::getComboBox();
    m_headingSequenceLevelCombo->setToolTip(tr("Base level to start heading sequence"));
    m_headingSequenceLevelCombo->addItem(tr("1"), 1);
    m_headingSequenceLevelCombo->addItem(tr("2"), 2);
    m_headingSequenceLevelCombo->addItem(tr("3"), 3);
    m_headingSequenceLevelCombo->addItem(tr("4"), 4);
    m_headingSequenceLevelCombo->addItem(tr("5"), 5);
    m_headingSequenceLevelCombo->addItem(tr("6"), 6);
    m_headingSequenceLevelCombo->setEnabled(false);

    connect(m_headingSequenceTypeCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, [this](int p_index){
                if (p_index > -1) {
                    HeadingSequenceType type = (HeadingSequenceType)m_headingSequenceTypeCombo->itemData(p_index).toInt();
                    m_headingSequenceLevelCombo->setEnabled(type != HeadingSequenceType::Disabled);
                }
            });

    QHBoxLayout *headingSequenceLayout = new QHBoxLayout();
    headingSequenceLayout->addWidget(m_headingSequenceTypeCombo);
    headingSequenceLayout->addWidget(m_headingSequenceLevelCombo);

    // Web Zoom Factor.
    m_customWebZoom = new QCheckBox(tr("Custom Web zoom factor"), this);
    m_customWebZoom->setToolTip(tr("Set the zoom factor of the Web page when reading"));
    m_webZoomFactorSpin = new QDoubleSpinBox(this);
    m_webZoomFactorSpin->setMaximum(c_webZoomFactorMax);
    m_webZoomFactorSpin->setMinimum(c_webZoomFactorMin);
    m_webZoomFactorSpin->setSingleStep(0.25);
    connect(m_customWebZoom, &QCheckBox::stateChanged,
            this, [this](int p_state){
                this->m_webZoomFactorSpin->setEnabled(p_state == Qt::Checked);
            });
    QHBoxLayout *zoomFactorLayout = new QHBoxLayout();
    zoomFactorLayout->addWidget(m_customWebZoom);
    zoomFactorLayout->addWidget(m_webZoomFactorSpin);

    // Color column.
    m_colorColumnEdit = new QLineEdit();
    m_colorColumnEdit->setToolTip(tr("Specify the screen column in fenced code block "
                                     "which will be highlighted"));
    QValidator *validator = new QRegExpValidator(QRegExp("\\d+"), this);
    m_colorColumnEdit->setValidator(validator);

    QLabel *colorColumnLabel = new QLabel(tr("Color column:"));
    colorColumnLabel->setToolTip(m_colorColumnEdit->toolTip());

    QFormLayout *mainLayout = new QFormLayout();
    mainLayout->addRow(tr("Note open mode:"), m_openModeCombo);
    mainLayout->addRow(tr("Heading sequence:"), headingSequenceLayout);
    mainLayout->addRow(zoomFactorLayout);
    mainLayout->addRow(colorColumnLabel, m_colorColumnEdit);

    setLayout(mainLayout);
}

bool VMarkdownTab::loadConfiguration()
{
    if (!loadOpenMode()) {
        return false;
    }

    if (!loadHeadingSequence()) {
        return false;
    }

    if (!loadWebZoomFactor()) {
        return false;
    }

    if (!loadColorColumn()) {
        return false;
    }

    return true;
}

bool VMarkdownTab::saveConfiguration()
{
    if (!saveOpenMode()) {
        return false;
    }

    if (!saveHeadingSequence()) {
        return false;
    }

    if (!saveWebZoomFactor()) {
        return false;
    }

    if (!saveColorColumn()) {
        return false;
    }

    return true;
}

bool VMarkdownTab::loadOpenMode()
{
    int mode = (int)g_config->getNoteOpenMode();
    bool found = false;
    for (int i = 0; i < m_openModeCombo->count(); ++i) {
        if (m_openModeCombo->itemData(i).toInt() == mode) {
            m_openModeCombo->setCurrentIndex(i);
            found = true;
            break;
        }
    }

    Q_ASSERT(found);
    return true;
}

bool VMarkdownTab::saveOpenMode()
{
    int mode = m_openModeCombo->currentData().toInt();
    g_config->setNoteOpenMode((OpenFileMode)mode);
    return true;
}

bool VMarkdownTab::loadHeadingSequence()
{
    HeadingSequenceType type = g_config->getHeadingSequenceType();
    int level = g_config->getHeadingSequenceBaseLevel();
    if (level < 1 || level > 6) {
        level = 1;
    }

    int idx = m_headingSequenceTypeCombo->findData((int)type);
    Q_ASSERT(idx > -1);
    m_headingSequenceTypeCombo->setCurrentIndex(idx);
    m_headingSequenceLevelCombo->setCurrentIndex(level - 1);
    m_headingSequenceLevelCombo->setEnabled(type != HeadingSequenceType::Disabled);

    return true;
}

bool VMarkdownTab::saveHeadingSequence()
{
    QVariant typeData = m_headingSequenceTypeCombo->currentData();
    Q_ASSERT(typeData.isValid());
    g_config->setHeadingSequenceType((HeadingSequenceType)typeData.toInt());
    g_config->setHeadingSequenceBaseLevel(m_headingSequenceLevelCombo->currentData().toInt());

    return true;
}

bool VMarkdownTab::loadWebZoomFactor()
{
    qreal factor = g_config->getWebZoomFactor();
    bool customFactor = g_config->isCustomWebZoomFactor();
    if (customFactor) {
        if (factor < c_webZoomFactorMin || factor > c_webZoomFactorMax) {
            factor = 1;
        }
        m_customWebZoom->setChecked(true);
        m_webZoomFactorSpin->setValue(factor);
    } else {
        m_customWebZoom->setChecked(false);
        m_webZoomFactorSpin->setValue(factor);
        m_webZoomFactorSpin->setEnabled(false);
    }

    return true;
}

bool VMarkdownTab::saveWebZoomFactor()
{
    if (m_customWebZoom->isChecked()) {
        g_config->setWebZoomFactor(m_webZoomFactorSpin->value());
    } else {
        g_config->setWebZoomFactor(-1);
    }

    return true;
}

bool VMarkdownTab::loadColorColumn()
{
    int colorColumn = g_config->getColorColumn();
    m_colorColumnEdit->setText(QString::number(colorColumn <= 0 ? 0 : colorColumn));
    return true;
}

bool VMarkdownTab::saveColorColumn()
{
    bool ok = false;
    int colorColumn = m_colorColumnEdit->text().toInt(&ok);
    if (ok && colorColumn >= 0) {
        g_config->setColorColumn(colorColumn);
    }

    return true;
}

