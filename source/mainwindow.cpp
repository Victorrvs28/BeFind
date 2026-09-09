#include "mainwindow.h"
#include <QDir>
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QDebug>
#include <vector>
#include <QString>
#include <QChar>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QUrl>
#include <fstream>
#include <QDir>
#include <QRegularExpression>
#include <algorithm>
#include <QDesktopServices>
#include <QMap>
#include "forms.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionadd_triggered()
{
    QString filepath = QFileDialog::getOpenFileName(this, tr("open item"), QDir::homePath(), tr("All files (*)"));

    if (filepath.isEmpty()) {
        return;
    }

    QString name = filepath;
    int lastBarIndex = name.lastIndexOf('/');
    name = name.mid(lastBarIndex + 1);

    int row = ui->table->rowCount();
    ui->table->insertRow(row);

    QTableWidgetItem* itname = new QTableWidgetItem(name);
    QTableWidgetItem* pathItem = new QTableWidgetItem(filepath);

    ui->table->setItem(row, 0, itname);
    ui->table->setItem(row, 2, pathItem);
}

void MainWindow::on_editAttributes_triggered()

{
    int selectedRow = ui->table->currentRow();
    if (selectedRow < 0) {
        return;
    }

    QString AttributesString = ui->table->item(selectedRow, 1)->text();
    AttributesString.replace('=', "");
    AttributesString.replace(", ", "");
   // QFile attributes(QDir::temp(), "attributesInput.tmp")
   // std::system("./editAttributes");

    QFile file(QDir::temp().absoluteFilePath("attributesOutput.tmp"));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (!line.isEmpty()) {
                attributes << line;
            }
        }
        file.close();
    }

    if (attributes.size() < 2) {
        return;
    }

    QString parsedList = "";
    for (int i = 0; i < attributes.size() - 1; i += 2) {
        parsedList += attributes[i];
        parsedList += " = ";
        parsedList += attributes[i + 1];
        parsedList += ", ";
    }

    if (parsedList.endsWith(", ")) {
        parsedList.chop(2);
    }

    QTableWidgetItem* item = new QTableWidgetItem(parsedList);
    ui->table->setItem(selectedRow, 1, item);
}

void MainWindow::on_searchButton_clicked()
{
    struct instruction {
        QString attribute = "";
        QString operatorr = "";
        QString value = "";
    };

    std::vector<QString> lexingVector;
    std::vector<instruction> instructionsVector;
    QString searchText = ui->searchBox->text();

    if (searchText.isEmpty()) {
        for (int row = 0; row < ui->table->rowCount(); row++) {
            ui->table->showRow(row);
        }
        return;
    }

    // Parse search text - split by spaces and &
    QStringList tokens = searchText.split(' ', Qt::SkipEmptyParts);

    // Build instructions from tokens
    for (int i = 0; i < tokens.size(); i += 3) {
        if (i + 2 < tokens.size()) {
            QString attr = tokens[i];
            QString op = tokens[i + 1];
            QString val = tokens[i + 2];

            // Remove trailing & if present
            if (attr.endsWith('&')) {
                attr.chop(1);
            }
            if (op.endsWith('&')) {
                op.chop(1);
            }
            if (val.endsWith('&')) {
                val.chop(1);
            }

            // Skip empty values
            if (!attr.isEmpty() && !op.isEmpty() && !val.isEmpty()) {
                instructionsVector.push_back({attr, op, val});
            }
        }
    }

    for (int row = 0; row < ui->table->rowCount(); row++) {
        struct attribute {
            QString attributeString;
            QString value;
        };

        std::vector<attribute> eachItemAttributes;

        // Parse attributes from table
        QTableWidgetItem *cellItem = ui->table->item(row, 1);
        if (cellItem) {
            QString text = cellItem->text();
            // Clean up the text
            text.replace(" = ", " ");
            text.replace(", ", " ");
            text = text.trimmed();

            QStringList attrTokens = text.split(' ', Qt::SkipEmptyParts);

            // Pair up attribute names and values
            for (int i = 0; i < attrTokens.size() - 1; i += 2) {
                if (i + 1 < attrTokens.size()) {
                    eachItemAttributes.push_back({attrTokens[i], attrTokens[i + 1]});
                }
            }
        }

        bool match = true;
        if (instructionsVector.empty()) {
            ui->table->setRowHidden(row, false);
            continue;
        }

        for (size_t i = 0; i < instructionsVector.size(); i++) {
            bool instructionMatched = false;
            auto boxAttribute = instructionsVector[i];

            if (boxAttribute.attribute == "find" && boxAttribute.operatorr == "content") {
                QTableWidgetItem *pathItem = ui->table->item(row, 2);
                if (pathItem) {
                    QString path = pathItem->text();
                    std::ifstream inFile(path.toStdString());

                    if (inFile.is_open()) {
                        std::string line;
                        while (std::getline(inFile, line)) {
                            if (QString::fromStdString(line).contains(boxAttribute.value)) {
                                instructionMatched = true;
                                break;
                            }
                        }
                    }
                }
            }
            else if (boxAttribute.attribute == "extension" && boxAttribute.operatorr == "is") {
                QTableWidgetItem *pathItem = ui->table->item(row, 2);
                if (pathItem) {
                    QString path = pathItem->text();
                    QString extension = QFileInfo(path).suffix();

                    if (boxAttribute.value.compare(extension, Qt::CaseInsensitive) == 0) {
                        instructionMatched = true;
                    }
                }
            }
            else if (boxAttribute.attribute == "name" && boxAttribute.operatorr == "is") {
                QString completeName = ui->table->item(row, 0)->text();
                QString name = QFileInfo(completeName).baseName();
                if (name == boxAttribute.value) {
                    instructionMatched = true;
                }
            }

            // Handle size operators
            const QString path = ui->table->item(row, 2)->text();
            const qint64 size = QFileInfo(path).size();
            QString inputSize = boxAttribute.value;

            if (boxAttribute.attribute == "KB") {
                if (boxAttribute.operatorr == ">") {
                    if (size > inputSize.toInt() * 1000) {
                        instructionMatched = true;
                    }
                }
                else if (boxAttribute.operatorr == "<") {
                    if (size < inputSize.toInt() * 1000) {
                        instructionMatched = true;
                    }
                }
            }
            else if (boxAttribute.attribute == "MB") {
                if (boxAttribute.operatorr == ">") {
                    if (size > inputSize.toInt() * 1000000) {
                        instructionMatched = true;
                    }
                }
                else if (boxAttribute.operatorr == "<") {
                    if (size < inputSize.toInt() * 1000000) {
                        instructionMatched = true;
                    }
                }
            }
            else if (boxAttribute.attribute == "GB") {
                if (boxAttribute.operatorr == ">") {
                    if (size > inputSize.toInt() * 1000000000) {
                        instructionMatched = true;
                    }
                }
                else if (boxAttribute.operatorr == "<") {
                    if (size < inputSize.toInt() * 1000000000) {
                        instructionMatched = true;
                    }
                }
            }
            else {
                // Check custom attributes
                for (size_t j = 0; j < eachItemAttributes.size(); j++) {
                    auto TableAttribute = eachItemAttributes[j];
                    if (TableAttribute.attributeString == boxAttribute.attribute) {
                        if (boxAttribute.operatorr == "=") {
                            if (TableAttribute.value == boxAttribute.value) {
                                instructionMatched = true;
                                break;
                            }
                        } else if (boxAttribute.operatorr == ">") {
                            if (TableAttribute.value.toInt() > boxAttribute.value.toInt()) {
                                instructionMatched = true;
                                break;
                            }
                        } else if (boxAttribute.operatorr == "<") {
                            if (TableAttribute.value.toInt() < boxAttribute.value.toInt()) {
                                instructionMatched = true;
                                break;
                            }
                        } else if (boxAttribute.operatorr == "!=") {
                            if (TableAttribute.value != boxAttribute.value) {
                                instructionMatched = true;
                                break;
                            }
                        }
                    }
                }
            }

            if (!instructionMatched) {
                match = false;
                break;
            }
        }

        ui->table->setRowHidden(row, !match);
    }
}

void MainWindow::on_actiondelete_triggered()
{
    int row = ui->table->currentRow();
    if (row < 0) {
        return;
    }
    ui->table->removeRow(row);
}

void MainWindow::on_actionnew_triggered()
{

}
void MainWindow::on_actionOFE_triggered()
{
    int row = ui->table->currentRow();
    QString file = ui->table->item(row, 2)->text();
    QDir folder = QFileInfo(file).absoluteDir();
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder.absolutePath()));
}

