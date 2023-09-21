#include "Calculator.h"
#include "ui_calculator.h"
#include <QDebug>
#include <QFile>
#include <QPdfWriter>
#include <QPainter>
#include <QMessageBox>
#include <QStyleFactory>
#include <functional>

#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

Calculator::Calculator(QWidget *parent) : QWidget(parent),
                                          mUi(new Ui::calculator)
{
    mUi->setupUi(this);
    this->setWindowTitle("Калькулятор");
    this->hide();

    if (mUi != NULL)
    {
        mUi->saveDish->setDisabled(true);
        // configuration calc table
        mUi->calcTableWidget->setRowCount(0);
        mUi->calcTableWidget->setColumnCount(6);

        mUi->calcTableWidget->setHorizontalHeaderLabels({"Название",
                                                         "Кол-во в", "Един. измерения",
                                                         "Цена за", "Един. измерения", "  "});

        QHeaderView *calcTableHeader = mUi->calcTableWidget->horizontalHeader();

        for (int col = 0, count = mUi->calcTableWidget->columnCount(); col < count; ++col)
        {
            calcTableHeader->setSectionResizeMode(col, QHeaderView::Stretch);
        }

        mUi->calcTableWidget->setEditTriggers(QAbstractItemView::DoubleClicked);

        // configuration ingridients table
        mUi->ingridientTableWidget->setRowCount(0);
        mUi->ingridientTableWidget->setColumnCount(2);
        mUi->ingridientTableWidget->setHorizontalHeaderLabels({"Название", " "});
        mUi->ingridientTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

        readFromFile();
        updateDataInIngridientsComboBox();

        connect(mUi->addIngridient, SIGNAL(clicked()), this, SLOT(addNewIngridient()));
        connect(mUi->calcCost, SIGNAL(clicked()), this, SLOT(calcCost()));
        connect(mUi->calcTabs, SIGNAL(currentChanged(int)), this, SLOT(changeTab(int)));
        connect(mUi->addIngridientToTable, SIGNAL(clicked()), this, SLOT(addIngridient()));
        connect(mUi->clearCalcTable, SIGNAL(clicked()), this, SLOT(clearCalcTable()));
        connect(mUi->saveDish, SIGNAL(clicked()), this, SLOT(writeToPDF()));
    }
}

Calculator::~Calculator()
{
    delete mUi;
}

void Calculator::addNewIngridient()
{
    const int rowCount = mUi->calcTableWidget->rowCount();

    mUi->calcTableWidget->insertRow(rowCount);
    mUi->calcTableWidget->setCellWidget(rowCount, 0, createIngridientsComboBox());
    mUi->calcTableWidget->setCellWidget(rowCount, 4, createPriceUnitsComboBox());
    mUi->calcTableWidget->setCellWidget(rowCount, 2, createQuantityUnitsComboBox());
    mUi->calcTableWidget->setCellWidget(rowCount, 5, createDeleteButton(TableID::CALC_TABLE));
}

void Calculator::calcCost()
{
    mDish.clear();
    const int rowCount = mUi->calcTableWidget->rowCount();
    const int colCount = mUi->calcTableWidget->columnCount();

    for (int row = 0; row < rowCount; ++row)
    {
        Ingredient rowData;
        for (int col = 0; col < colCount; ++col)
        {
            const auto *item = mUi->calcTableWidget->item(row, col);
            const auto *comboBox = qobject_cast<QComboBox *>(mUi->calcTableWidget->cellWidget(row, col));
            if (item || comboBox)
            {
                switch (col)
                {
                case 0:
                    rowData.name = comboBox->currentText().toStdString();
                    break;
                case 1:
                    rowData.quantity = item->text().toFloat();
                    break;
                case 2:
                    rowData.quantityUnit = comboBox->itemData(comboBox->currentIndex()).value<QuantityUnit>();
                    break;
                case 3:
                    rowData.price = item->text().toFloat();
                    break;
                case 4:
                    rowData.priceUnit = comboBox->itemData(comboBox->currentIndex()).value<PriceUnit>();
                    break;
                default:
                    break;
                }
            }
        }
        bool checkUnit = checkUnitSimile(rowData.priceUnit, rowData.quantityUnit);

        if (!checkUnit)
        {
            qobject_cast<QComboBox *>(mUi->calcTableWidget->cellWidget(row, 2))->setStyleSheet("border:1px solid red;");
            qobject_cast<QComboBox *>(mUi->calcTableWidget->cellWidget(row, 4))->setStyleSheet("border:1px solid red;");
            QMessageBox::critical(this, "Ошибка", "Неверные единицы измерения");
        }
        else
        {
            qobject_cast<QComboBox *>(mUi->calcTableWidget->cellWidget(row, 2))->setStyle(QStyleFactory::create("Windows"));
            qobject_cast<QComboBox *>(mUi->calcTableWidget->cellWidget(row, 2))->setPalette(qApp->style()->standardPalette());
            qobject_cast<QComboBox *>(mUi->calcTableWidget->cellWidget(row, 4))->setStyle(QStyleFactory::create("Windows"));
            qobject_cast<QComboBox *>(mUi->calcTableWidget->cellWidget(row, 4))->setPalette(qApp->style()->standardPalette());
            mDish.push_back(rowData);
            mUi->saveDish->setDisabled(false);
        }
    }

    double productCost = 0;

    for (int i = 0; i < mDish.size(); i++)
    {
        const auto &item = mDish[i];
        float quantity = item.quantity;
        float price = item.price;

        double rowCost = (quantity * price) / coefficient(item.priceUnit, item.quantityUnit);

        productCost += rowCost;
    }

    mUi->productCost->setText(QString::number(productCost) + " грн.");
}

QComboBox *Calculator::createQuantityUnitsComboBox()
{
    auto comboBox = new QComboBox(mUi->calcTableWidget);
    comboBox->addItem("кг", QVariant::fromValue(QuantityUnit::KILOGRAMM));
    comboBox->addItem("г", QVariant::fromValue(QuantityUnit::GRAMM));
    comboBox->addItem("л", QVariant::fromValue(QuantityUnit::LITER));
    comboBox->addItem("мл", QVariant::fromValue(QuantityUnit::MILLILITER));
    comboBox->addItem("шт", QVariant::fromValue(QuantityUnit::UNIT));

    return comboBox;
}

QComboBox *Calculator::createPriceUnitsComboBox()
{
    auto comboBox = new QComboBox(mUi->calcTableWidget);
    comboBox->addItem("грн/кг", QVariant::fromValue(PriceUnit::UAH_PER_KG));
    comboBox->addItem("грн/л", QVariant::fromValue(PriceUnit::UAH_PER_LITER));
    comboBox->addItem("грн/шт", QVariant::fromValue(PriceUnit::UAH_PER_UNIT));

    return comboBox;
}

QComboBox *Calculator::createIngridientsComboBox()
{
    auto comboBox = new QComboBox();
    for (const auto &item : mIngredients)
    {
        comboBox->addItem(item);
    }

    return comboBox;
}

QPushButton *Calculator::createDeleteButton(TableID tableId)
{
    auto deleteButton = new QPushButton("Удалить");
    // TODO update path to icon
    deleteButton->setIcon(QIcon(":/icons/remove.png"));
    deleteButton->setIconSize(QSize(24, 24));

    connect(deleteButton, &QPushButton::clicked, this, [this, tableId]()
            { tableId == TableID::CALC_TABLE
                  ? onDeleteCalcButtonClicked()
                  : onDeleteIngridientButtonClicked(); });

    return deleteButton;
}

void Calculator::onDeleteCalcButtonClicked()
{
    QPushButton *button = qobject_cast<QPushButton *>(sender());

    if (button)
    {
        int rowIndex = mUi->calcTableWidget->indexAt(button->pos()).row();

        qDebug() << "rowIndex: " << rowIndex;

        mUi->calcTableWidget->removeRow(rowIndex);

        disconnect(button, &QPushButton::clicked, this, nullptr);
    }
}

void Calculator::onDeleteIngridientButtonClicked()
{
    QPushButton *button = qobject_cast<QPushButton *>(sender());

    if (button)
    {
        int rowIndex = mUi->ingridientTableWidget->indexAt(button->pos()).row();

        qDebug() << "rowIndex: " << rowIndex;

        mUi->ingridientTableWidget->removeRow(rowIndex);

        mIngredients.erase(mIngredients.begin() + rowIndex);

        writeToFile();

        disconnect(button, &QPushButton::clicked, this, nullptr);
    }
}

/*
            | UAH_PER_KG	UAH_PER_LITER	UAH_PER_UNIT
KILOGRAMM	| 1             PO              N/A
GRAMM	    | 1000          PO              N/A
LITER	    | PO            1               N/A
MILLILITER	| PO            1000            N/A
UNIT	    | N/A           N/A             1

*/

float Calculator::coefficient(PriceUnit priceUnit, QuantityUnit quantityUnit) const
{
    float result = 1.0;

    if (priceUnit == PriceUnit::UAH_PER_KG)
    {
        if (quantityUnit == QuantityUnit::KILOGRAMM)
            result = 1.0;
        else if (quantityUnit == QuantityUnit::GRAMM)
            result = 1000.0;
        // TODO
    }
    else if (priceUnit == PriceUnit::UAH_PER_LITER)
    {
        if (quantityUnit == QuantityUnit::LITER)
            result = 1.0;
        else if (quantityUnit == QuantityUnit::MILLILITER)
            result = 1000.0;
        // TODO
    }
    else if (priceUnit == PriceUnit::UAH_PER_UNIT)
    {
        if (quantityUnit == QuantityUnit::UNIT)
            result = 1.0;
        // TODO
    }

    return result;
}

void Calculator::changeTab(int index)
{
    if (index == 0)
    {
        qDebug() << "index: " << index;
        updateDataInIngridientsComboBox();
    }
}

void Calculator::updateDataInIngridientsComboBox()
{
    const int rowCount = mUi->calcTableWidget->rowCount();
    const int colCount = mUi->calcTableWidget->columnCount();

    for (int row = 0; row < rowCount; ++row)
    {
        for (int col = 0; col < colCount; ++col)
        {
            auto *comboBox = qobject_cast<QComboBox *>(mUi->calcTableWidget->cellWidget(row, col));
            if (comboBox && col == 0)
            {
                comboBox->clear();
                for (const auto &item : mIngredients)
                {
                    comboBox->addItem(item);
                }
            }
        }
    }
}

void Calculator::addIngridient()
{
    if (!mUi->ingridientInput->toPlainText().isEmpty())
    {
        const int rowCount = mUi->ingridientTableWidget->rowCount();
        mUi->ingridientTableWidget->insertRow(rowCount);

        QTableWidgetItem *newItem = new QTableWidgetItem(mUi->ingridientInput->toPlainText());

        mUi->ingridientTableWidget->setItem(rowCount, 0, newItem);
        mUi->ingridientTableWidget->setCellWidget(rowCount, 1, createDeleteButton(TableID::INGRIDIENTS_TABLE));

        mIngredients.push_back(mUi->ingridientInput->toPlainText());

        writeToFile();

        mUi->ingridientInput->clear();
    }
}

void Calculator::writeToFile()
{
    QFile file("ingredients.txt");

    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
        for (const auto &ingredient : mIngredients)
        {
            out << ingredient << "\n";
        }
        file.close();
    }
}

void Calculator::readFromFile()
{
    QFile file("ingredients.txt");

    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);

        while (!in.atEnd())
        {
            QString line = in.readLine();
            mIngredients.push_back(line);
        }
        file.close();

        updateIngridientTable();
    }
}

void Calculator::updateIngridientTable()
{
    for (const auto &item : mIngredients)
    {
        const int rowCount = mUi->ingridientTableWidget->rowCount();
        mUi->ingridientTableWidget->insertRow(rowCount);
        mUi->ingridientTableWidget->setItem(rowCount, 0, new QTableWidgetItem(item));
        mUi->ingridientTableWidget->setCellWidget(rowCount, 1, createDeleteButton(TableID::INGRIDIENTS_TABLE));
    }
}

void Calculator::clearCalcTable()
{
    int rowCount = mUi->calcTableWidget->rowCount();

    mUi->saveDish->setDisabled(true);

    for (int i = 0; i < rowCount; i++)
    {
        mUi->calcTableWidget->removeRow(0);
        auto button = qobject_cast<QPushButton *>(mUi->calcTableWidget->cellWidget(0, 5));
        disconnect(button, &QPushButton::clicked, this, nullptr);
    }
}

void Calculator::writeToPDF()
{
    QMessageBox::information(this, "Сохранение", "Рецепт сохранен");

    QPdfWriter pdfWriter("example.pdf");
    pdfWriter.setPageSize(QPagedPaintDevice::A4);
    QPainter painter(&pdfWriter);

    painter.drawText(100, 100, "Список ингредиентов:");
    painter.drawText(100, 300, "");

    QTableWidget table;
    table.setColumnCount(5); // Количество столбцов
    table.setHorizontalHeaderLabels({"Ингредиент", "Количество", "Един. изм", "Цена", "Един. изм"});
    table.setRowCount(mDish.size());

    const int rowHeight = 450; // Установите желаемую высоту строки
    const int colWidth = 1700; // Установите желаемую ширину столбца
    table.verticalHeader()->setDefaultSectionSize(rowHeight);
    for (int col = 0; col < table.columnCount(); ++col)
    {
        table.setColumnWidth(col, colWidth);
    }

    table.setContentsMargins(150, 500, 0, 0);

    QFont font = table.font();
    font.setPointSize(120); // Установите желаемый размер шрифта
    table.setFont(font);

    for (int row = 0; row < mDish.size(); ++row)
    {
        const Ingredient &ingredient = mDish[row];

        // QTableWidgetItem *item = new QTableWidgetItem(ingredient.name);

        // Устанавливаем выравнивание текста в ячейке слева
        // item->setTextAlignment(Qt::AlignVCenter);
        // table.setItem(row, 0, item);
        table.setItem(row, 1, new QTableWidgetItem(QString::number(ingredient.quantity)));
        table.setItem(row, 2, new QTableWidgetItem(QString::number(static_cast<int>(ingredient.quantityUnit))));
        table.setItem(row, 3, new QTableWidgetItem(QString::number(ingredient.price)));
        table.setItem(row, 4, new QTableWidgetItem(QString::number(static_cast<int>(ingredient.priceUnit))));
    }

    // Устанавливаем размер таблицы
    const qreal scale = 20.5; // Увеличьте масштаб, чтобы увеличить размер таблицы
    table.resize(table.size() * scale);

    // Рисуем таблицу на PDF-странице
    table.render(&painter);

    painter.end();


    QJsonObject dish;

    dish.insert("name", "MyName");

    QJsonArray ingridients;

    for (int row = 0; row < mDish.size(); ++row)
    {
        const Ingredient &ingred= mDish[row];
        QJsonObject ingridient;
        ingridient.insert("наименование", QString::fromUtf8(ingred.name.c_str()));
        ingridient.insert("цена", ingred.price);
        ingridients.append(ingridient);
    }
    dish.insert("ингридиенты", ingridients);

    QJsonDocument doc;
    doc.setObject(dish);
    QFile file("out.json");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        file.write(doc.toJson());
    }
    file.close();
}

bool Calculator::checkUnitSimile(PriceUnit priceUnit, QuantityUnit quantityUnit)
{
    switch (quantityUnit)
    {
    case QuantityUnit::UNIT: return priceUnit == PriceUnit::UAH_PER_UNIT;
    case QuantityUnit::LITER:
    case QuantityUnit::MILLILITER: return priceUnit == PriceUnit::UAH_PER_LITER;
    case QuantityUnit::GRAMM:
    case QuantityUnit::KILOGRAMM: return priceUnit == PriceUnit::UAH_PER_KG;
    default: return false;
    }
}
