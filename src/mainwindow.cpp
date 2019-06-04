#include <QErrorMessage>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QString>
#include <QStringListModel>

#include "mainwindow.h"
#include "sbml.h"
#include "simulate.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  connect(ui->pltPlot,
          SIGNAL(plottableClick(QCPAbstractPlottable *, int, QMouseEvent *)),
          this, SLOT(on_graphClicked(QCPAbstractPlottable *, int)));

  // for debugging convenience: import a model and an image on startup
  // <debug>
  sbml_doc.loadFile("test.xml");
  sbml_doc.importGeometry("two-blobs-100x100.bmp");
  ui->lblGeometry->setImage(sbml_doc.compartment_image);
  update_ui();
  // </debug>
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_action_About_triggered() {
  QMessageBox msgBox;
  msgBox.setWindowTitle("About");
  QString info("Spatial Model Editor\n");
  info.append("github.com/lkeegan/spatial-model-editor\n\n");
  info.append("Included libraries:\n");
  info.append("\nQt5:\t");
  info.append(QT_VERSION_STR);
  info.append("\nlibSBML:\t");
  info.append(libsbml::getLibSBMLDottedVersion());
  for (const auto &dep : {"expat", "libxml", "xerces-c", "bzip2", "zip"}) {
    if (libsbml::isLibSBMLCompiledWith(dep) != 0) {
      info.append("\n");
      info.append(dep);
      info.append(":\t");
      info.append(libsbml::getLibSBMLDependencyVersionOf(dep));
    }
  }
  msgBox.setText(info);
  msgBox.exec();
}

void MainWindow::on_actionE_xit_triggered() { QApplication::quit(); }

void MainWindow::on_action_Open_SBML_file_triggered() {
  // load SBML file
  QString filename = QFileDialog::getOpenFileName();
  if (!filename.isEmpty()) {
    sbml_doc.loadFile(qPrintable(filename));
    if (sbml_doc.isValid) {
      update_ui();
    }
  }
}

void MainWindow::update_ui() {
  // update raw XML display
  ui->txtSBML->setText(sbml_doc.xml);

  // update list of compartments
  ui->listCompartments->clear();
  ui->listCompartments->insertItems(0, sbml_doc.compartments);

  // update list of reactions
  ui->listReactions->clear();
  ui->listReactions->insertItems(0, sbml_doc.reactions);

  // update list of functions
  ui->listFunctions->clear();
  ui->listFunctions->insertItems(0, sbml_doc.functions);

  // update tree list of species
  ui->listSpecies->clear();
  QList<QTreeWidgetItem *> items;
  for (auto c : sbml_doc.compartments) {
    // add compartments as top level items
    QTreeWidgetItem *comp =
        new QTreeWidgetItem(ui->listSpecies, QStringList({c}));
    ui->listSpecies->addTopLevelItem(comp);
    for (auto s : sbml_doc.species[c]) {
      // add each species as child of compartment
      comp->addChild(new QTreeWidgetItem(comp, QStringList({s})));
    }
  }
  ui->listSpecies->expandAll();
}

void MainWindow::on_action_Save_SBML_file_triggered() {
  QMessageBox msgBox;
  msgBox.setText("todo");
  msgBox.exec();
}

void MainWindow::on_actionAbout_Qt_triggered() { QMessageBox::aboutQt(this); }

void MainWindow::on_actionGeometry_from_image_triggered() {
  QString filename =
      QFileDialog::getOpenFileName(this, "Import geometry from image", "",
                                   "Image Files (*.png *.jpg *.bmp)");
  sbml_doc.importGeometry(filename);
  ui->lblGeometry->setImage(sbml_doc.compartment_image);
}

void MainWindow::on_lblGeometry_mouseClicked() {
  QRgb col = ui->lblGeometry->getColour();
  if (waiting_for_compartment_choice) {
    // update compartment geometry (i.e. colour) of selected compartment to the
    // one the user just clicked on
    auto new_comp = ui->listCompartments->selectedItems()[0]->text();
    QRgb old_col = sbml_doc.compartment_to_colour[new_comp];
    if (old_col != 0) {
      // if there was already a colour assigned to this compartment, assign the
      // colour to a null compartment
      sbml_doc.colour_to_compartment[old_col] = "";
    }
    auto old_comp = sbml_doc.colour_to_compartment[col];
    qDebug("old comp: %s, new comp: %s", qPrintable(old_comp),
           qPrintable(new_comp));
    if (old_comp != "") {
      // if the new colour was already assigned to another compartment, set the
      // colour of that compartment to null
      sbml_doc.compartment_to_colour[old_comp] = 0;
    }
    sbml_doc.colour_to_compartment[col] = new_comp;
    sbml_doc.compartment_to_colour[new_comp] = col;
    // update display by simulating user click on listCompartments
    on_listCompartments_currentTextChanged(new_comp);
    waiting_for_compartment_choice = false;
  } else {
    // display compartment the user just clicked on
    auto items = ui->listCompartments->findItems(
        sbml_doc.colour_to_compartment[col], Qt::MatchExactly);
    if (!items.empty()) {
      ui->listCompartments->setCurrentRow(ui->listCompartments->row(items[0]));
    }
  }
}

void MainWindow::on_chkSpeciesIsSpatial_stateChanged(int arg1) {
  ui->grpSpatial->setEnabled(arg1);
}

void MainWindow::on_chkShowSpatialAdvanced_stateChanged(int arg1) {
  ui->grpSpatialAdavanced->setEnabled(arg1);
}

void MainWindow::on_listReactions_currentTextChanged(
    const QString &currentText) {
  ui->listProducts->clear();
  ui->listReactants->clear();
  ui->listReactionParams->clear();
  ui->lblReactionRate->clear();
  if (currentText.size() > 0) {
    qDebug() << currentText;
    const auto *reac = sbml_doc.model->getReaction(qPrintable(currentText));
    for (unsigned i = 0; i < reac->getNumProducts(); ++i) {
      ui->listProducts->addItem(reac->getProduct(i)->getSpecies().c_str());
    }
    for (unsigned i = 0; i < reac->getNumReactants(); ++i) {
      ui->listReactants->addItem(reac->getReactant(i)->getSpecies().c_str());
    }
    for (unsigned i = 0; i < reac->getKineticLaw()->getNumParameters(); ++i) {
      ui->listReactionParams->addItem(
          reac->getKineticLaw()->getParameter(i)->getId().c_str());
    }
    ui->lblReactionRate->setText(reac->getKineticLaw()->getFormula().c_str());
  }
}

void MainWindow::sim1d() {
  // do simple simulation of model

  // compile reaction expressions
  simulate sim(sbml_doc);
  sim.compile_reactions();
  // set initial concentrations
  for (unsigned int i = 0; i < sbml_doc.model->getNumSpecies(); ++i) {
    const auto *spec = sbml_doc.model->getSpecies(i);
    // if SBML file specifies amount: convert to concentration
    if (spec->isSetInitialAmount()) {
      double vol =
          sbml_doc.model->getCompartment(spec->getCompartment())->getSize();
      sim.species_values[i] = spec->getInitialAmount() / vol;
    } else {
      sim.species_values[i] = spec->getInitialConcentration();
    }
  }

  // generate vector of resulting concentrations at each timestep
  sim.timestep_1d_euler(0.0);
  QVector<double> time;
  double t = 0;
  double dt = ui->txtSimDt->text().toDouble();
  std::vector<QVector<double>> conc(sim.species_values.size());
  time.push_back(t);
  for (std::size_t i = 0; i < sim.species_values.size(); ++i) {
    conc[i].push_back(sim.species_values[i]);
  }
  for (int i = 0;
       i < static_cast<int>(ui->txtSimLength->text().toDouble() / dt); ++i) {
    // do an euler integration timestep
    sim.timestep_1d_euler(dt);
    t += dt;
    time.push_back(t);
    for (std::size_t i = 0; i < sim.species_values.size(); ++i) {
      conc[i].push_back(sim.species_values[i]);
    }
  }
  // plot results
  ui->pltPlot->clearGraphs();
  ui->pltPlot->setInteraction(QCP::iRangeDrag, true);
  ui->pltPlot->setInteraction(QCP::iRangeZoom, true);
  ui->pltPlot->setInteraction(QCP::iSelectPlottables, true);

  std::vector<QColor> col{{0, 0, 0},       {230, 25, 75},   {60, 180, 75},
                          {255, 225, 25},  {0, 130, 200},   {245, 130, 48},
                          {145, 30, 180},  {70, 240, 240},  {240, 50, 230},
                          {210, 245, 60},  {250, 190, 190}, {0, 128, 128},
                          {230, 190, 255}, {170, 110, 40},  {255, 250, 200},
                          {128, 0, 0},     {170, 255, 195}, {128, 128, 0},
                          {255, 215, 180}, {0, 0, 128},     {128, 128, 128}};
  ui->pltPlot->legend->setVisible(true);
  for (std::size_t i = 0; i < sim.species_values.size(); ++i) {
    ui->pltPlot->addGraph();
    ui->pltPlot->graph(static_cast<int>(i))->setData(time, conc[i]);
    ui->pltPlot->graph(static_cast<int>(i))->setPen(col[i]);
    ui->pltPlot->graph(static_cast<int>(i))
        ->setName(sbml_doc.speciesID[i].c_str());
  }
  ui->pltPlot->xAxis->setLabel("time");
  ui->pltPlot->xAxis->setRange(time.front(), time.back());
  ui->pltPlot->yAxis->setLabel("concentration");
  ui->pltPlot->replot();
}

void MainWindow::on_btnSimulate_clicked() {
  // simple 2d simulation

  // initialise concentration field from current compartment
  field species_field(1, ui->lblGeometry->getImage(),
                      ui->lblGeometry->getColour());
  ui->lblGeometry->setImage(species_field.compartment_image());
  // set initial concentration
  species_field.conc[17] = 50;
  species_field.conc[18] = 40;
  species_field.conc[19] = 39;
  species_field.conc[20] = 50;
  species_field.conc[21] = 40;
  species_field.conc[22] = 39;
  species_field.conc[23] = 50;
  species_field.conc[24] = 40;
  species_field.conc[25] = 39;
  // display initial concentration
  ui->lblGeometry->setImage(species_field.concentration_image(0));
  // compile reaction expressions
  simulate sim(sbml_doc);
  sim.compile_reactions();
  // do euler integration
  QVector<double> time;
  QVector<double> conc;
  double t = 0;
  double dt = ui->txtSimDt->text().toDouble();
  for (int i = 0;
       i < static_cast<int>(ui->txtSimLength->text().toDouble() / dt); ++i) {
    t += dt;
    species_field.diffusion_op();
    sim.timestep_2d_euler(species_field, dt);
    images.push_back(species_field.concentration_image(0).copy());
    conc.push_back(species_field.get_mean_concentration(0));
    time.push_back(t);
    ui->lblGeometry->setImage(images.back());
  }
  // plot results
  ui->pltPlot->clearGraphs();
  ui->pltPlot->setInteraction(QCP::iRangeDrag, true);
  ui->pltPlot->setInteraction(QCP::iRangeZoom, true);
  ui->pltPlot->setInteraction(QCP::iSelectPlottables, true);
  ui->pltPlot->addGraph();
  ui->pltPlot->graph(0)->setData(time, conc);
  ui->pltPlot->replot();
}

void MainWindow::on_listFunctions_currentTextChanged(
    const QString &currentText) {
  ui->listFunctionParams->clear();
  ui->lblFunctionDef->clear();
  if (currentText.size() > 0) {
    qDebug() << currentText;
    const auto *func =
        sbml_doc.model->getFunctionDefinition(qPrintable(currentText));
    for (unsigned i = 0; i < func->getNumArguments(); ++i) {
      ui->listFunctionParams->addItem(
          libsbml::SBML_formulaToL3String(func->getArgument(i)));
    }
    ui->lblFunctionDef->setText(
        libsbml::SBML_formulaToL3String(func->getBody()));
  }
}

void MainWindow::on_listSpecies_itemActivated(QTreeWidgetItem *item,
                                              int column) {
  // if user selects a species (i.e. an item with a parent)
  if ((item != nullptr) && (item->parent() != nullptr)) {
    qDebug() << item->text(column);
    // display species information
    auto *spec = sbml_doc.model->getSpecies(qPrintable(item->text(column)));
    ui->txtInitialConcentration->setText(
        QString::number(spec->getInitialConcentration()));
    if ((spec->isSetConstant() && spec->getConstant()) ||
        (spec->isSetBoundaryCondition() && spec->getBoundaryCondition())) {
      ui->chkSpeciesIsConstant->setCheckState(Qt::CheckState::Checked);
    } else {
      ui->chkSpeciesIsConstant->setCheckState(Qt::CheckState::Unchecked);
    }
  }
}

void MainWindow::on_listSpecies_itemClicked(QTreeWidgetItem *item, int column) {
  on_listSpecies_itemActivated(item, column);
}

void MainWindow::on_graphClicked(QCPAbstractPlottable *plottable,
                                 int dataIndex) {
  double dataValue = plottable->interface1D()->dataMainValue(dataIndex);
  QString message =
      QString("Clicked on graph '%1' at data point #%2 with value %3.")
          .arg(plottable->name())
          .arg(dataIndex)
          .arg(dataValue);
  qDebug() << message;
  ui->lblGeometry->setImage(images[dataIndex]);
}

void MainWindow::on_btnChangeCompartment_clicked() {
  waiting_for_compartment_choice = true;
}

void MainWindow::on_listCompartments_currentTextChanged(
    const QString &currentText) {
  ui->txtCompartmentSize->clear();
  if (currentText.size() > 0) {
    qDebug() << currentText;
    const auto *comp = sbml_doc.model->getCompartment(qPrintable(currentText));
    ui->txtCompartmentSize->setText(QString::number(comp->getSize()));
    QRgb col = sbml_doc.compartment_to_colour[currentText];
    qDebug() << qAlpha(col);
    if (col == 0) {
      // null (transparent white) RGB colour: compartment does not have
      // an assigned colour in the image
      ui->lblCompartmentColour->setPalette(QPalette());
      ui->lblCompartmentColour->setText("none");
      ui->lblCompShape->setPixmap(QPixmap());
      ui->lblCompShape->setText("none");
    } else {
      // update colour box
      QPalette palette;
      palette.setColor(QPalette::Window, QColor::fromRgb(col));
      ui->lblCompartmentColour->setPalette(palette);
      ui->lblCompartmentColour->setText("");
      // update image mask
      ui->lblGeometry->setMaskColour(col);
      QPixmap pixmap = QPixmap::fromImage(ui->lblGeometry->getMask());
      ui->lblCompShape->setPixmap(pixmap);
      ui->lblCompShape->setText("");
    }
  }
}