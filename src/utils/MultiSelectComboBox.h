#pragma once

#include <QComboBox>
#include <QListWidget>

class MultiSelectComboBox : public QComboBox
{
    Q_OBJECT

public:
    MultiSelectComboBox(QWidget* aParent = Q_NULLPTR);
    void addItem(const QString& aText, const QVariant& aUserData = QVariant(), bool default_checked = false);
    void addItems(const QStringList& aTexts); 
    void removeItem(const QVariant& aUserData);
    QList<QVariant>* getItems();

    QSet<int> getSelectedItems();
    QStringList currentText();
    int count() const;
    void hidePopup() override;

    void hide();
    void SetSearchBarPlaceHolderText(const QString& aPlaceHolderText);
    void SetPlaceHolderText(const QString& aPlaceHolderText);
    void ResetSelection();
    QVariant itemData(int index);
    void setDisabled(bool disable);
    int getHighLight();
	QVariant getHighLightData();
    void setText(const QString& aText);
    QString getItemText(int idx);
    
    void selectIndex(int idx, bool selected);
    void setIndexSelect(int idx, bool selected);
    void selectItem(const QVariant& aUserData, bool selected);
    bool isSelected(const QVariant& aUserData);
    bool isSelected(int i);
    void setHighLight(int idx, bool highlight);
    int findData(const QVariant& aUserData);

signals:
    void highLightSelect(int index,bool is_highlight);
    void selectionChanged(int item_idx,bool is_selected);

public slots:
    void clear();
    void setCurrentText(const QString& aText);
    void setCurrentText(const QStringList& aText);

protected:
    void wheelEvent(QWheelEvent* aWheelEvent) override;
    bool eventFilter(QObject* aObject, QEvent* aEvent) override;
    void keyPressEvent(QKeyEvent* aEvent) override;

private:
    void stateChanged(int aState);
    void onSearch(const QString& aSearchString);
    void itemClicked(int aIndex);

    QListWidget* mListWidget;
    QLineEdit* mLineEdit;
    QList<QVariant>* m_list_custom_info;
    QSet<int>* current_selected_items;
    int highLight = -1;
    bool is_disabled = false;
};