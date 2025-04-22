
#include "MultiSelectComboBox.h"
#include <QLineEdit>
#include <QCheckBox>
#include <QEvent>

namespace {
    const int scSearchBarIndex = 0;
}

MultiSelectComboBox::MultiSelectComboBox(QWidget* aParent) :
    QComboBox(aParent),
    mListWidget(new QListWidget(this)),
    mLineEdit(new SilentLineEdit(this)),
    m_list_custom_info(new QList<QVariant>),
    current_selected_items(new QSet<int>)
{


    mLineEdit->setReadOnly(true);
    connect(mLineEdit, &SilentLineEdit::leftClicked, this, &MultiSelectComboBox::showPopup);

    setModel(mListWidget->model());
    setView(mListWidget);
    setLineEdit(mLineEdit);

    connect(this, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &MultiSelectComboBox::itemClicked);
}

void MultiSelectComboBox::hidePopup()
{
    int width = this->width();
    int height = mListWidget->height();
    int x = QCursor::pos().x() - mapToGlobal(geometry().topLeft()).x() + geometry().x();
    int y = QCursor::pos().y() - mapToGlobal(geometry().topLeft()).y() + geometry().y();
    if (x >= 0 && x <= width && y >= this->height() && y <= height + this->height())
    {
        // Item was clicked, do not hide popup
    }
    else
    {
        QComboBox::hidePopup();
    }
}

void MultiSelectComboBox::hide()
{
    QComboBox::hidePopup();
}

void MultiSelectComboBox::stateChanged(int aState)
{
    Q_UNUSED(aState);
    QString selectedData("");
    int count = mListWidget->count();

    int sender_idx = -1;
    QCheckBox* sender = static_cast<QCheckBox*>(QObject::sender());

    for (int i = 0; i < count; ++i)
    {
        QWidget* widget = mListWidget->itemWidget(mListWidget->item(i));
        QCheckBox* checkBox = static_cast<QCheckBox*>(widget);

        if (checkBox == sender)
        {
            sender_idx = i;
        }
    }
    if (!sender->isChecked()) {
        if (highLight != sender_idx || is_disabled) {
            sender->blockSignals(true);
            sender->setCheckState(Qt::Checked);
            sender->blockSignals(false);


            if (highLight != sender_idx) {
                // dehighlight previous highlight item  and highlight current
                if (highLight != -1) {
                    emit highLightSelect(highLight, false);
                }                
                highLight = sender_idx;
                setText(getItemText(sender_idx));
                emit highLightSelect(sender_idx,true); //sender-1 must is already selected
            }
            else if(is_disabled){
                // dehighlight previous highlight item and do nothing
                setText(QString("%1/%2 Selected")
                    .arg(getSelectedItems().size()).arg(m_list_custom_info->size()));                
                if (highLight != -1) {
                    emit highLightSelect(highLight, false);
                    highLight = -1;
                }
            }
        }
        else {
            // deselect current item and unhighlight all
            if (highLight != -1) {
                emit highLightSelect(highLight, false);
                highLight = -1;
            }
            current_selected_items->remove(sender_idx);
            setText(QString("%1/%2 Selected")
                .arg(getSelectedItems().size()).arg(m_list_custom_info->size()));
            emit selectionChanged(sender_idx, false);
        }
    }
    else {
        if (is_disabled) {
            sender->blockSignals(true);
            sender->setCheckState(Qt::Unchecked);
            sender->blockSignals(false);

            setText(QString("%1/%2 Selected")
                .arg(getSelectedItems().size()).arg(m_list_custom_info->size()));
            // unhighlight previous highlight item and do nothing
            if (highLight != -1) {
                emit highLightSelect(highLight, false);
                highLight = -1;
            }
        }
        else {
            // first select and high light item
            if (highLight != -1) {
                emit highLightSelect(highLight, false);
            }
            if (highLight != sender_idx) {
                highLight = sender_idx;
                emit highLightSelect(highLight, true);
            }
            current_selected_items->insert(sender_idx);
            setText(getItemText(sender_idx));
            emit selectionChanged(sender_idx, true);
        }
    }
}

void MultiSelectComboBox::setHighLight(int idx, bool is_hightlight) {
    if (!is_hightlight) {
        highLight = -1;
        emit highLightSelect(idx, false);
        setText(QString("%1/%2 Selected")
            .arg(getSelectedItems().size()).arg(m_list_custom_info->size()));
    }
    else if(highLight != idx && is_hightlight){
        if (highLight != -1) {
            emit highLightSelect(highLight, false);
        }
        highLight = idx;
        emit highLightSelect(idx, true);
        setText(getItemText(idx));
    }
}
void MultiSelectComboBox::addItem(const QString& aText, const QVariant& aUserData, bool default_checked )
{
    m_list_custom_info->append(aUserData);
    QListWidgetItem* listWidgetItem = new QListWidgetItem(mListWidget);
    QCheckBox* checkBox = new QCheckBox(this);
    checkBox->setText(aText);
    if (default_checked) {
        checkBox->setCheckState(Qt::Checked);;
        current_selected_items->insert(mListWidget->count()-1);
    }
    mListWidget->addItem(listWidgetItem);
    mListWidget->setItemWidget(listWidgetItem, checkBox);
    connect(checkBox, &QCheckBox::stateChanged, this, &MultiSelectComboBox::stateChanged);
    setText(QString("%1/%2 Selected")
        .arg(getSelectedItems().size()).arg(m_list_custom_info->size()));
}

void MultiSelectComboBox::removeItem(const QVariant& aUserData)
{
    auto i = m_list_custom_info->indexOf(aUserData);
    if (i < 0)return;
    m_list_custom_info->remove(i);
    if(current_selected_items->contains(i))
        current_selected_items->remove(i );
    //QWidget* widget = mListWidget->itemWidget(mListWidget->item(i ));
    //QCheckBox* checkBox = static_cast<QCheckBox*>(widget);
    //checkBox->disconnect(this);
    auto item = mListWidget->takeItem(i);
    delete item;
    QComboBox::hidePopup();
    //mListWidget->removeItemWidget(mListWidget->item(i ));
    //this->update();
    setText(QString("%1/%2 Selected")
        .arg(getSelectedItems().size()).arg(m_list_custom_info->size()));
}

QList<QVariant>* MultiSelectComboBox::getItems()
{
    return m_list_custom_info;
}
int MultiSelectComboBox::findData(const QVariant& aUserData)
{
    return m_list_custom_info->indexOf(aUserData);
}

QStringList MultiSelectComboBox::currentText()
{
    QStringList emptyStringList;
    if (!mLineEdit->text().isEmpty())
    {
        emptyStringList = mLineEdit->text().split(';');
    }
    return emptyStringList;
}

QVariant MultiSelectComboBox::itemData(int index) {
    return (*m_list_custom_info)[index];
}
void MultiSelectComboBox::setDisabled(bool disable)
{
    is_disabled = disable;
}
int MultiSelectComboBox::getHighLight()
{
    return highLight;
}
QVariant MultiSelectComboBox::getHighLightData()
{
    return itemData(highLight);
}
void MultiSelectComboBox::setText(const QString& aText)
{
    mLineEdit->setText(aText);
}
QString MultiSelectComboBox::getItemText(int idx)
{
    QWidget* widget = mListWidget->itemWidget(mListWidget->item(idx));
    QCheckBox* checkBox = static_cast<QCheckBox*>(widget);
    return checkBox->text();
}

//select and hightlight
void MultiSelectComboBox::selectIndex(int i, bool selected)
{
    if (i < 0)
        return;
    QWidget* widget = mListWidget->itemWidget(mListWidget->item(i));
    QCheckBox* checkBox = static_cast<QCheckBox*>(widget);
    if (checkBox->isChecked() != selected) {

        checkBox->blockSignals(true);
        if(selected)
            checkBox->setCheckState(Qt::Checked);
        else
            checkBox->setCheckState(Qt::Unchecked);
        checkBox->blockSignals(false);
        if (selected)
            current_selected_items->insert(i);
        else
            current_selected_items->remove(i);
        if (selected) {
            if ( highLight != i) {
                if(highLight!= -1)
                    emit highLightSelect(highLight, false);
                highLight = i;
                emit highLightSelect(highLight, true);
                setText(getItemText(highLight));
            }
        }
        else {
            if (highLight == i) {
				emit highLightSelect(highLight, false);
				highLight = -1;
                setText(QString("%1/%2 Selected")
                    .arg(getSelectedItems().size()).arg(m_list_custom_info->size()));

            }
        }
    }
    else {
        if (selected && highLight != i) {
            if(highLight != -1)
                emit highLightSelect(highLight, false);
            emit highLightSelect(i, true);
            highLight = i;
            setText(getItemText(highLight));
        }
    }
}

// only select
void MultiSelectComboBox::setIndexSelect(int i, bool selected)
{
    if (i < 0)
        return;
    QWidget* widget = mListWidget->itemWidget(mListWidget->item(i));
    QCheckBox* checkBox = static_cast<QCheckBox*>(widget);
    if (checkBox->isChecked() != selected) {
        checkBox->blockSignals(true);
        if (selected)
            checkBox->setCheckState(Qt::Checked);
        else
            checkBox->setCheckState(Qt::Unchecked);
        checkBox->blockSignals(false);
        if (selected)
            current_selected_items->insert(i);
        else
            current_selected_items->remove(i);
        if (!selected && i == highLight) {
            emit highLightSelect(highLight, false);
            highLight = -1;
        }
    }
}

//select and hightlight
void MultiSelectComboBox::selectItem(const QVariant& aUserData ,bool selected)
{
    auto i = m_list_custom_info->indexOf(aUserData);
    selectIndex(i, selected);
}
bool MultiSelectComboBox::isSelected(const QVariant& aUserData)
{
	auto i = m_list_custom_info->indexOf(aUserData);
    return isSelected(i);
}
bool MultiSelectComboBox::isSelected(int i)
{
    if (i < 0)
        return false;
    QWidget* widget = mListWidget->itemWidget(mListWidget->item(i));
    QCheckBox* checkBox = static_cast<QCheckBox*>(widget);
    return checkBox->isChecked();
}
void MultiSelectComboBox::addItems(const QStringList& aTexts)
{
    for (const auto& string : aTexts)
    {
        addItem(string);
    }
}

QSet<int> MultiSelectComboBox::getSelectedItems()
{
    return *current_selected_items;
}

int MultiSelectComboBox::count() const
{
    int count = mListWidget->count();// Do not count the search bar
    if (count < 0)
    {
        count = 0;
    }
    return count;
}

void MultiSelectComboBox::onSearch(const QString& aSearchString)
{
    for (int i = 1; i < mListWidget->count(); i++)
    {
        QCheckBox* checkBox = static_cast<QCheckBox*>(mListWidget->itemWidget(mListWidget->item(i)));
        if (checkBox->text().contains(aSearchString, Qt::CaseInsensitive))
        {
            mListWidget->item(i)->setHidden(false);
        }
        else
        {
            mListWidget->item(i)->setHidden(true);
        }
    }
}


void MultiSelectComboBox::itemClicked(int aIndex)
{
    if (aIndex != scSearchBarIndex)// 0 means the search bar
    {
        QWidget* widget = mListWidget->itemWidget(mListWidget->item(aIndex));
        QCheckBox* checkBox = static_cast<QCheckBox*>(widget);
        checkBox->setChecked(!checkBox->isChecked());
    }
}

void MultiSelectComboBox::SetPlaceHolderText(const QString& aPlaceHolderText)
{
    mLineEdit->setPlaceholderText(aPlaceHolderText);
}

void MultiSelectComboBox::clear()
{
    highLight = -1;
    is_disabled = false;
    mListWidget->clear();
    m_list_custom_info->clear();
    current_selected_items->clear();
    setText(QString("0/0 Selected"));
}

void MultiSelectComboBox::wheelEvent(QWheelEvent* aWheelEvent)
{
    // Do not handle the wheel event
    Q_UNUSED(aWheelEvent);
}


void MultiSelectComboBox::keyPressEvent(QKeyEvent* aEvent)
{
    // Do not handle key event
    Q_UNUSED(aEvent);
}

void MultiSelectComboBox::setCurrentText(const QString& aText)
{
    Q_UNUSED(aText);
}

void MultiSelectComboBox::setCurrentText(const QStringList& aText)
{
    int count = mListWidget->count();

    for (int i = 1; i < count; ++i)
    {
        QWidget* widget = mListWidget->itemWidget(mListWidget->item(i));
        QCheckBox* checkBox = static_cast<QCheckBox*>(widget);
        QString checkBoxString = checkBox->text();
        if (aText.contains(checkBoxString))
        {
            checkBox->setChecked(true);
        }
    }
}

void MultiSelectComboBox::ResetSelection()
{
    int count = mListWidget->count();

    for (int i = 1; i < count; ++i)
    {
        QWidget* widget = mListWidget->itemWidget(mListWidget->item(i));
        QCheckBox* checkBox = static_cast<QCheckBox*>(widget);
        checkBox->setChecked(false);
    }
}