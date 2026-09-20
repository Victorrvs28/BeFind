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
#include <QProcess>
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
    QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        tr("Open item"),
        QDir::homePath(),
        tr("All files (*)")
        );

    if (filePaths.isEmpty()) {
        return;
    }

    for (const QString &filePath : filePaths) {
        QString name = filePath;
        int lastBarIndex = name.lastIndexOf('/');
        name = name.mid(lastBarIndex + 1);

        int row = ui->table->rowCount();
        ui->table->insertRow(row);

        QTableWidgetItem* nameItem = new QTableWidgetItem(name);
        QTableWidgetItem* pathItem = new QTableWidgetItem(filePath);

        ui->table->setItem(row, 0, nameItem);
        ui->table->setItem(row, 2, pathItem);
                                            }
}

void MainWindow::on_editAttributes_triggered()
{
    if (ui->table->currentRow() == -1) {
        return;
    }

    int row = ui->table->currentRow();

    if (ui->table->item(row, 1) == nullptr) {
        ui->table->setItem(row, 1, new QTableWidgetItem(""));
    }

    QString inputAttributes = ui->table->item(row, 1)->text();

    // Converts "attribute = value, " to "attribute value"
    inputAttributes.replace(" = ", " ");
    inputAttributes.replace(", ", " ");

    QFile inputFile("input.tmp");

    if (!inputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Could not create input.tmp";
        return;
                                                                }

    {
        QTextStream inputFileStream(&inputFile);
        const QStringList tokens = inputAttributes.split(' ', Qt::SkipEmptyParts);

        for (const QString &token : tokens) {
            inputFileStream << token << ' ';
        }
    }

    inputFile.close(); // Essential: makes sure the data is written before running the external process

    QString editAttributesPath =
        QCoreApplication::applicationDirPath() + "/editAttributes";

    std::system(("chmod +x " + editAttributesPath.toStdString()).c_str());
    std::system(editAttributesPath.toStdString().c_str());

    QFile outputFile("output.tmp");

    if (!outputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not read output.tmp";
        return;
                                                                }

    QTextStream outputFileStream(&outputFile);
    QString outputFileContent = outputFileStream.readAll();
    outputFile.close();

    QStringList tokens = outputFileContent.split(' ', Qt::SkipEmptyParts);
    QString result;

    for (int i = 0; i + 1 < tokens.size(); i += 2) {
        result += tokens[i] + " = " + tokens[i + 1] + ", ";
                                                    }

    result.chop(2);
    ui->table->item(row, 1)->setText(result);
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

    QStringList tokens = searchText.split(' ', Qt::SkipEmptyParts);

    for (int i = 0; i < tokens.size(); i += 3) {
        if (i + 2 < tokens.size()) {
            QString attribute = tokens[i];
            QString operatorr = tokens[i + 1];
            QString value = tokens[i + 2];

            if (attribute.endsWith('&')) {
                attribute.chop(1);
                                        }

            if (operatorr.endsWith('&')) {
                operatorr.chop(1);
                                            }

            if (value.endsWith('&')) {
                value.chop(1);
                                        }

            if (!attribute.isEmpty() && !operatorr.isEmpty() && !value.isEmpty()) {
                instructionsVector.push_back({attribute, operatorr, value});
                                                                                  }
                                    }
                                                    }

    for (int row = 0; row < ui->table->rowCount(); row++) {
        struct attribute {
            QString attributeString;
            QString value;
        };

        std::vector<attribute> eachItemAttributes;

        // Parse attributes from the table
        QTableWidgetItem *cellItem = ui->table->item(row, 1);

        if (cellItem) {
            QString text = cellItem->text();

            // Clean up the text
            text.replace(" = ", " ");
            text.replace(", ", " ");
            text = text.trimmed();

            QStringList attributeTokens = text.split(' ', Qt::SkipEmptyParts);

            // Pair attribute names and values
            for (int i = 0; i < attributeTokens.size() - 1; i += 2) {
                if (i + 1 < attributeTokens.size()) {
                    eachItemAttributes.push_back({
                        attributeTokens[i],
                        attributeTokens[i + 1]
                                                });
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

            if (boxAttribute.attribute == "find" &&
                boxAttribute.operatorr == "content") {

                QTableWidgetItem *pathItem = ui->table->item(row, 2);

                if (pathItem) {
                    QString path = pathItem->text();
                    std::ifstream inputFile(path.toStdString());

                    if (inputFile.is_open()) {
                        std::string line;

                        while (std::getline(inputFile, line)) {
                            if (QString::fromStdString(line).contains(boxAttribute.value)) {
                                instructionMatched = true;
                                break;
                                                                                            }
                                                                }
                                                }
                                }
                                                    }
            else if (boxAttribute.attribute == "extension" &&
                     boxAttribute.operatorr == "is") {

                QTableWidgetItem *pathItem = ui->table->item(row, 2);

                if (pathItem) {
                    QString path = pathItem->text();
                    QString extension = QFileInfo(path).suffix();

                    if (boxAttribute.value.compare(extension, Qt::CaseInsensitive) == 0) {
                        instructionMatched = true;
                                                                                            }
                            }
                                                    }
            else if (boxAttribute.attribute == "name" &&
                     boxAttribute.operatorr == "is") {

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
                    auto tableAttribute = eachItemAttributes[j];

                    if (tableAttribute.attributeString == boxAttribute.attribute) {
                        if (boxAttribute.operatorr == "=") {
                            if (tableAttribute.value == boxAttribute.value) {
                                instructionMatched = true;
                                break;
                                                                            }
                                                            }
                        else if (boxAttribute.operatorr == ">") {
                            if (tableAttribute.value.toInt() > boxAttribute.value.toInt()) {
                                instructionMatched = true;
                                break;
                                                                                            }
                                                                }
                        else if (boxAttribute.operatorr == "<") {
                            if (tableAttribute.value.toInt() < boxAttribute.value.toInt()) {
                                instructionMatched = true;
                                break;
                                                                                            }
                                                                }
                        else if (boxAttribute.operatorr == "!=") {
                            if (tableAttribute.value != boxAttribute.value) {
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
    ui->table->setRowCount(0);
    m_currentFilePath.clear();
    setWindowTitle(tr("New file"));
}

void MainWindow::on_actionOFE_triggered() // Open in file manager
{
    int row = ui->table->currentRow();
    QString file = ui->table->item(row, 2)->text();

    QDir folder = QFileInfo(file).absoluteDir();
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder.absolutePath()));
}

bool MainWindow::saveToFile(const QString &filePath)
{
    QFile file(filePath);
    QTextStream outputStream(&file);

    for (int row = 0; row < ui->table->rowCount(); row++) {
        QString name = ui->table->item(row, 0)
        ? ui->table->item(row, 0)->text()
        : "";

        QString attributes = ui->table->item(row, 1)
                                 ? ui->table->item(row, 1)->text()
                                 : "";

        QString path = ui->table->item(row, 2)
                           ? ui->table->item(row, 2)->text()
                           : "";

        outputStream << name << '\t'
                     << attributes << '\t'
                     << path << '\n';
                                                            }

    file.close();
    return true;
}

void MainWindow::on_actionsave_triggered()
{
    if (m_currentFilePath.isEmpty()) {
        on_actionsave_as_triggered();
        return;
                                    }

    saveToFile(m_currentFilePath);
}

void MainWindow::on_actionsave_as_triggered()
{
    QString filePath = QFileDialog::getSaveFileName(this,tr("Save list as"),QDir::homePath(),tr("File lists (*.txt)"));

    if (filePath.isEmpty()) {
        return;
                            }

    if (saveToFile(filePath)) {
        m_currentFilePath = filePath;
        setWindowTitle(QFileInfo(filePath).fileName());
                                }
}

void MainWindow::on_actionopen_triggered()
{
    QString filePath = QFileDialog::getOpenFileName(this,tr("Open list"),QDir::homePath(),tr("File lists (*.txt)"));

    if (filePath.isEmpty()) {
        return;
                            }

    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not open file:" << filePath;
        return;
                                                            }

    ui->table->setRowCount(0);

    QTextStream inputStream(&file);

    while (!inputStream.atEnd()) {
        QString line = inputStream.readLine();

        if (line.isEmpty()) {
            continue;
                            }

        QStringList fields = line.split('\t');

        if (fields.size() != 3) {
            continue;
                                }

        int row = ui->table->rowCount();
        ui->table->insertRow(row);

        ui->table->setItem(row, 0, new QTableWidgetItem(fields[0]));
        ui->table->setItem(row, 1, new QTableWidgetItem(fields[1]));
        ui->table->setItem(row, 2, new QTableWidgetItem(fields[2]));
                                    }

    file.close();

    m_currentFilePath = filePath;
    setWindowTitle(QFileInfo(filePath).fileName());
}
