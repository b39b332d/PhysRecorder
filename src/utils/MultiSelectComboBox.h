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
    QSet<int> getSelectedItems();
    QStringList currentText();
    int count() const;
    void hidePopup() override;
    void SetSearchBarPlaceHolderText(const QString& aPlaceHolderText);
    void SetPlaceHolderText(const QString& aPlaceHolderText);
    void ResetSelection();
    QVariant itemData(int index);
    void setDisabled(bool disable);
    int getHightLight();
    void setText(const QString& aText);
    QString getItemText(int idx);
    

signals:
    void heightSelect(int index,bool is_highlight);
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
    QLineEdit* mSearchBar;
    QList<QVariant>* m_list_custom_info;
    QSet<int>* current_selected_items;
    int hightLight = -1;
    bool is_disabled = false;
};