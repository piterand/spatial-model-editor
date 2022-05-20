#include "dialogoptcost.hpp"
#include "dialogconcentrationimage.hpp"
#include "ui_dialogoptcost.h"

static QString toQStr(double x) { return QString::number(x, 'g', 14); }

static int toIndex(sme::simulate::OptCostType optCostType) {
  switch (optCostType) {
  case sme::simulate::OptCostType::Concentration:
    return 0;
  case sme::simulate::OptCostType::ConcentrationDcdt:
    return 1;
  default:
    return 0;
  }
}

static sme::simulate::OptCostType toOptCostTypeEnum(int index) {
  switch (index) {
  case 0:
    return sme::simulate::OptCostType::Concentration;
  case 1:
    return sme::simulate::OptCostType::ConcentrationDcdt;
  default:
    return sme::simulate::OptCostType::Concentration;
  }
}

static int toIndex(sme::simulate::OptCostDiffType optCostDiffType) {
  switch (optCostDiffType) {
  case sme::simulate::OptCostDiffType::Absolute:
    return 0;
  case sme::simulate::OptCostDiffType::Relative:
    return 1;
  default:
    return 0;
  }
}

static sme::simulate::OptCostDiffType toOptCostDiffTypeEnum(int index) {
  switch (index) {
  case 0:
    return sme::simulate::OptCostDiffType::Absolute;
  case 1:
    return sme::simulate::OptCostDiffType::Relative;
  default:
    return sme::simulate::OptCostDiffType::Absolute;
  }
}

DialogOptCost::DialogOptCost(
    const sme::model::Model &model,
    const std::vector<sme::simulate::OptCost> &defaultOptCosts,
    const sme::simulate::OptCost *initialOptCost, QWidget *parent)
    : model{model}, defaultOptCosts{defaultOptCosts},
      QDialog(parent), ui{std::make_unique<Ui::DialogOptCost>()} {
  ui->setupUi(this);
  int cmbIndex{0};
  if (defaultOptCosts.empty()) {
    // model has no species: disable everything except cancel button
    ui->cmbSpecies->setEnabled(false);
    ui->cmbDiffType->setEnabled(false);
    ui->cmbCostType->setEnabled(false);
    ui->txtSimulationTime->setEnabled(false);
    ui->btnEditTargetValues->setEnabled(false);
    ui->txtScaleFactor->setEnabled(false);
    ui->txtEpsilon->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
  } else {
    if (initialOptCost != nullptr) {
      optCost = *initialOptCost;
    } else {
      optCost = defaultOptCosts.front();
    }
    ui->cmbCostType->setCurrentIndex(toIndex(optCost.optCostType));
    ui->cmbDiffType->setCurrentIndex(toIndex(optCost.optCostDiffType));
    ui->txtSimulationTime->setText(QString::number(optCost.simulationTime));
    ui->txtScaleFactor->setText(QString::number(optCost.scaleFactor));
    ui->txtEpsilon->setText(QString::number(optCost.epsilon));
    ui->txtEpsilon->setEnabled(optCost.optCostDiffType ==
                               sme::simulate::OptCostDiffType::Relative);
    for (const auto &defaultOptCost : defaultOptCosts) {
      ui->cmbSpecies->addItem(defaultOptCost.name);
      if (defaultOptCost.name == optCost.name) {
        cmbIndex = ui->cmbSpecies->count() - 1;
      }
    }
    connect(ui->cmbSpecies, &QComboBox::currentIndexChanged, this,
            &DialogOptCost::cmbSpecies_currentIndexChanged);
    connect(ui->cmbCostType, &QComboBox::currentIndexChanged, this,
            &DialogOptCost::cmbCostType_currentIndexChanged);
    connect(ui->cmbDiffType, &QComboBox::currentIndexChanged, this,
            &DialogOptCost::cmbDiffType_currentIndexChanged);
    connect(ui->txtSimulationTime, &QLineEdit::editingFinished, this,
            &DialogOptCost::txtSimulationTime_editingFinished);
    connect(ui->btnEditTargetValues, &QPushButton::clicked, this,
            &DialogOptCost::btnEditTargetValues_clicked);
    connect(ui->txtScaleFactor, &QLineEdit::editingFinished, this,
            &DialogOptCost::txtScaleFactor_editingFinished);
    connect(ui->txtEpsilon, &QLineEdit::editingFinished, this,
            &DialogOptCost::txtEpsilon_editingFinished);
    ui->cmbSpecies->setCurrentIndex(cmbIndex);
  }
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogOptCost::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogOptCost::reject);
}

DialogOptCost::~DialogOptCost() = default;

[[nodiscard]] const sme::simulate::OptCost &DialogOptCost::getOptCost() const {
  return optCost;
}

void DialogOptCost::cmbSpecies_currentIndexChanged(int index) {
  auto i{static_cast<std::size_t>(index)};
  if (defaultOptCosts[i].name != optCost.name) {
    optCost = defaultOptCosts[i];
    ui->cmbCostType->setCurrentIndex(toIndex(optCost.optCostType));
    ui->cmbDiffType->setCurrentIndex(toIndex(optCost.optCostDiffType));
    ui->txtSimulationTime->setText(QString::number(optCost.simulationTime));
    ui->txtScaleFactor->setText(QString::number(optCost.scaleFactor));
    ui->txtEpsilon->setText(QString::number(optCost.epsilon));
    ui->txtEpsilon->setEnabled(optCost.optCostDiffType ==
                               sme::simulate::OptCostDiffType::Relative);
  }
}

void DialogOptCost::cmbCostType_currentIndexChanged(int index) {
  optCost.optCostType = toOptCostTypeEnum(index);
}

void DialogOptCost::cmbDiffType_currentIndexChanged(int index) {
  optCost.optCostDiffType = toOptCostDiffTypeEnum(index);
  ui->txtEpsilon->setEnabled(optCost.optCostDiffType ==
                             sme::simulate::OptCostDiffType::Relative);
}

void DialogOptCost::txtSimulationTime_editingFinished() {
  optCost.simulationTime = ui->txtSimulationTime->text().toDouble();
  ui->txtSimulationTime->setText(toQStr(optCost.simulationTime));
}

void DialogOptCost::btnEditTargetValues_clicked() {
  DialogConcentrationImage dialog(optCost.targetValues,
                                  model.getSpeciesGeometry(optCost.id),
                                  model.getDisplayOptions().invertYAxis);
  if (dialog.exec() == QDialog::Accepted) {
    optCost.targetValues = dialog.getConcentrationArray();
  }
}

void DialogOptCost::txtScaleFactor_editingFinished() {
  optCost.scaleFactor = ui->txtScaleFactor->text().toDouble();
  ui->txtScaleFactor->setText(toQStr(optCost.scaleFactor));
}

void DialogOptCost::txtEpsilon_editingFinished() {
  optCost.epsilon = ui->txtEpsilon->text().toDouble();
  ui->txtEpsilon->setText(toQStr(optCost.epsilon));
}
