#include "sbml.hpp"

#include <fstream>

#include <QFile>

#include "catch.hpp"

#include "sbml_test_data/very_simple_model.hpp"
#include "sbml_test_data/yeast_glycolysis.hpp"

void createSBMLlvl2doc(const std::string &filename) {
  std::unique_ptr<libsbml::SBMLDocument> document(
      new libsbml::SBMLDocument(2, 4));
  // create model
  auto *model = document->createModel();
  // create two compartments of different size
  for (int i = 0; i < 2; ++i) {
    auto *comp = model->createCompartment();
    comp->setId("compartment" + std::to_string(i));
    comp->setSize(1e-10 * i);
  }
  // create 2 species inside first compartment with initialConcentration set
  for (int i = 0; i < 2; ++i) {
    auto *spec = model->createSpecies();
    spec->setId("spec" + std::to_string(i) + "c0");
    spec->setCompartment("compartment0");
    spec->setInitialConcentration(i * 1e-12);
  }
  // create 3 species inside second compartment with initialAmount set
  for (int i = 0; i < 3; ++i) {
    auto *spec = model->createSpecies();
    spec->setId("spec" + std::to_string(i) + "c1");
    spec->setCompartment("compartment1");
    spec->setInitialAmount(100 * i);
  }
  // create a reaction: spec0c0 -> spec1c0
  auto *reac = model->createReaction();
  reac->setId("reac1");
  reac->addProduct(model->getSpecies("spec1c0"));
  reac->addReactant(model->getSpecies("spec0c0"));
  auto *kin = model->createKineticLaw();
  kin->setFormula("5*spec0c0");
  reac->setKineticLaw(kin);
  // write SBML document to file
  libsbml::SBMLWriter().writeSBML(document.get(), filename);
}

SCENARIO("import SBML doc without geometry", "[sbml][non-gui]") {
  // create simple SBML level 2.4 model
  createSBMLlvl2doc("tmp.xml");
  // import SBML model
  sbml::SbmlDocWrapper s;
  s.importSBMLFile("tmp.xml");
  REQUIRE(s.isValid == true);
  // export it again
  s.exportSBMLFile("tmp.xml");
  THEN("upgrade SBML doc and add default 2d spatial geometry") {
    // load new model
    std::unique_ptr<libsbml::SBMLDocument> doc(
        libsbml::readSBMLFromFile("tmp.xml"));
    REQUIRE(doc != nullptr);
    auto *model = doc->getModel();
    REQUIRE(model != nullptr);
    REQUIRE(model->getLevel() == 3);
    REQUIRE(model->getVersion() == 2);

    REQUIRE(doc->isPackageEnabled("spatial") == true);
    auto *plugin = dynamic_cast<libsbml::SpatialModelPlugin *>(
        model->getPlugin("spatial"));
    REQUIRE(plugin != nullptr);
    REQUIRE(plugin->isSetGeometry() == true);
    auto *geom = plugin->getGeometry();
    REQUIRE(geom != nullptr);
    REQUIRE(geom->getNumCoordinateComponents() == 2);
    REQUIRE(geom->getCoordinateComponent(0)->getType() ==
            libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_X);
    REQUIRE(geom->getCoordinateComponent(1)->getType() ==
            libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Y);

    REQUIRE(geom->getNumGeometryDefinitions() == 1);
    REQUIRE(geom->getGeometryDefinition(0)->isSampledFieldGeometry() == true);
    REQUIRE(geom->getGeometryDefinition(0)->getIsActive() == true);
    auto *sfgeom = dynamic_cast<libsbml::SampledFieldGeometry *>(
        geom->getGeometryDefinition(0));
    REQUIRE(sfgeom != nullptr);
    for (unsigned i = 0; i < model->getNumCompartments(); ++i) {
      auto *comp = model->getCompartment(i);
      auto *scp = dynamic_cast<libsbml::SpatialCompartmentPlugin *>(
          comp->getPlugin("spatial"));
      REQUIRE(scp->isSetCompartmentMapping() == true);
      std::string domainTypeID = scp->getCompartmentMapping()->getDomainType();
      REQUIRE(geom->getDomainByDomainType(domainTypeID) ==
              geom->getDomain(comp->getId() + "_domain"));
      REQUIRE(sfgeom->getSampledVolumeByDomainType(domainTypeID)->getId() ==
              comp->getId() + "_sampledVolume");
    }
  }
  WHEN("import geometry & assign compartments") {
    // import geometry image & assign compartments to colours
    s.importGeometryFromImage(":/geometry/single-pixels-3x1.png");
    s.setCompartmentColour("compartment0", 0xffaaaaaa);
    s.setCompartmentColour("compartment1", 0xff525252);
    // export it again
    s.exportSBMLFile("tmp.xml");

    std::unique_ptr<libsbml::SBMLDocument> doc(
        libsbml::readSBMLFromFile("tmp.xml"));
    auto *model = doc->getModel();
    auto *plugin = dynamic_cast<libsbml::SpatialModelPlugin *>(
        model->getPlugin("spatial"));
    auto *geom = plugin->getGeometry();
    auto *sfgeom = dynamic_cast<libsbml::SampledFieldGeometry *>(
        geom->getGeometryDefinition(0));
    std::vector<int> sfvals;
    auto *sf = geom->getSampledField(sfgeom->getSampledField());
    sf->getSamples(sfvals);
    REQUIRE(sf->getNumSamples1() == 3);
    REQUIRE(sf->getNumSamples2() == 1);
    CAPTURE(static_cast<unsigned>(sfvals[0]));
    CAPTURE(static_cast<unsigned>(sfvals[1]));
    CAPTURE(static_cast<unsigned>(sfvals[2]));

    auto *scp0 = dynamic_cast<libsbml::SpatialCompartmentPlugin *>(
        model->getCompartment("compartment0")->getPlugin("spatial"));
    REQUIRE(scp0->isSetCompartmentMapping() == true);
    auto *sfvol0 = sfgeom->getSampledVolumeByDomainType(
        scp0->getCompartmentMapping()->getDomainType());
    CAPTURE(sfvol0->getSampledValue());
    REQUIRE(static_cast<unsigned>(sfvol0->getSampledValue()) == 0xffaaaaaa);
    REQUIRE(static_cast<unsigned>(sfvol0->getSampledValue()) ==
            static_cast<unsigned>(sfvals[1]));

    auto *scp1 = dynamic_cast<libsbml::SpatialCompartmentPlugin *>(
        model->getCompartment("compartment1")->getPlugin("spatial"));
    REQUIRE(scp1->isSetCompartmentMapping() == true);
    auto *sfvol1 = sfgeom->getSampledVolumeByDomainType(
        scp1->getCompartmentMapping()->getDomainType());
    CAPTURE(sfvol1->getSampledValue());
    REQUIRE(static_cast<unsigned>(sfvol1->getSampledValue()) == 0xff525252);
    REQUIRE(static_cast<unsigned>(sfvol1->getSampledValue()) ==
            static_cast<unsigned>(sfvals[2]));

    WHEN("import concentration & set diff constants") {
      // import concentration
      s.importConcentrationFromImage("spec0c0",
                                     ":/geometry/single-pixels-3x1.png");
      // spec0c0 concentration set to 1 (default rescaling)
      // -> c0 pixel (1,0) has default species colour (230,25,75)
      // -> other pixels transparent
      REQUIRE(s.getConcentrationImage("spec0c0").size() == QSize(3, 1));
      REQUIRE(s.getConcentrationImage("spec0c0").pixel(1, 0) ==
              QColor(230, 25, 75).rgba());
      REQUIRE(s.getConcentrationImage("spec0c0").pixel(0, 0) == 0x00000000);
      REQUIRE(s.getConcentrationImage("spec0c0").pixel(2, 0) == 0x00000000);
      // set spec1c1conc to zero -> black pixel
      QImage img(":/geometry/single-pixels-3x1.png");
      img.fill(0xff000000);
      img.save("tmp.png");
      s.importConcentrationFromImage("spec1c1", "tmp.png");
      REQUIRE(s.getConcentrationImage("spec1c1").pixel(0, 0) == 0x00000000);
      REQUIRE(s.getConcentrationImage("spec1c1").pixel(1, 0) == 0x00000000);
      REQUIRE(s.getConcentrationImage("spec1c1").pixel(2, 0) == 0xff000000);
      // set and then re-set spec2c1conc
      s.importConcentrationFromImage("spec2c1", "tmp.png");
      REQUIRE(s.getConcentrationImage("spec2c1").pixel(0, 0) == 0x00000000);
      REQUIRE(s.getConcentrationImage("spec2c1").pixel(1, 0) == 0x00000000);
      REQUIRE(s.getConcentrationImage("spec2c1").pixel(2, 0) == 0xff000000);
      img.fill(0xff221321);
      img.save("tmp.png");
      s.importConcentrationFromImage("spec2c1", "tmp.png");
      REQUIRE(s.getConcentrationImage("spec2c1").pixel(0, 0) == 0x00000000);
      REQUIRE(s.getConcentrationImage("spec2c1").pixel(1, 0) == 0x00000000);
      REQUIRE(s.getConcentrationImage("spec2c1").pixel(2, 0) ==
              QColor(245, 130, 48).rgba());
      s.setIsSpatial("spec0c0", true);
      s.setIsSpatial("spec1c0", true);
      s.setIsSpatial("spec0c1", true);
      s.setDiffusionConstant("spec0c0", 0.123);
      s.setDiffusionConstant("spec1c0", 0.1);
      s.setDiffusionConstant("spec1c0", 0.999999);
      s.setDiffusionConstant("spec0c1", 23.1 + 1e-12);
      CAPTURE(s.getDiffusionConstant("spec0c0"));
      CAPTURE(s.getDiffusionConstant("spec1c0"));
      CAPTURE(s.getDiffusionConstant("spec0c1"));
      REQUIRE(s.getDiffusionConstant("spec0c0") == dbl_approx(0.123));
      REQUIRE(s.getDiffusionConstant("spec1c0") == dbl_approx(0.999999));
      REQUIRE(s.getDiffusionConstant("spec0c1") == dbl_approx(23.1 + 1e-12));

      // export model
      s.exportSBMLFile("tmp2.xml");
      // import model again, recover concentration & compartment assignments
      sbml::SbmlDocWrapper s2;
      s2.importSBMLFile("tmp2.xml");
      REQUIRE(s2.getCompartmentColour("compartment0") == 0xffaaaaaa);
      REQUIRE(s2.getCompartmentColour("compartment1") == 0xff525252);
      REQUIRE(s2.getConcentrationImage("spec0c0").pixel(1, 0) ==
              QColor(230, 25, 75).rgba());
      REQUIRE(s2.getConcentrationImage("spec0c0").pixel(0, 0) == 0x00000000);
      REQUIRE(s2.getConcentrationImage("spec0c0").pixel(2, 0) == 0x00000000);
      REQUIRE(s2.getConcentrationImage("spec1c1").pixel(0, 0) == 0x00000000);
      REQUIRE(s2.getConcentrationImage("spec1c1").pixel(1, 0) == 0x00000000);
      REQUIRE(s2.getConcentrationImage("spec1c1").pixel(2, 0) == 0xff000000);
      REQUIRE(s2.getConcentrationImage("spec2c1").pixel(0, 0) == 0x00000000);
      REQUIRE(s2.getConcentrationImage("spec2c1").pixel(1, 0) == 0x00000000);
      REQUIRE(s2.getConcentrationImage("spec2c1").pixel(2, 0) ==
              QColor(245, 130, 48).rgba());

      CAPTURE(s2.getDiffusionConstant("spec0c0"));
      CAPTURE(s2.getDiffusionConstant("spec1c0"));
      CAPTURE(s2.getDiffusionConstant("spec0c1"));
      REQUIRE(s2.getDiffusionConstant("spec0c0") == dbl_approx(0.123));
      REQUIRE(s2.getDiffusionConstant("spec1c0") == dbl_approx(0.999999));
      REQUIRE(s2.getDiffusionConstant("spec0c1") == dbl_approx(23.1 + 1e-12));
    }
  }
}

SCENARIO("import SBML level 2 document", "[sbml][non-gui]") {
  // create simple SBML level 2.4 model
  createSBMLlvl2doc("tmp.xml");
  // import SBML model
  sbml::SbmlDocWrapper s;
  s.importSBMLFile("tmp.xml");
  REQUIRE(s.isValid == true);

  // import geometry image & assign compartments to colours
  s.importGeometryFromImage(":/geometry/single-pixels-3x1.png");
  s.setCompartmentColour("compartment0", 0xffaaaaaa);
  s.setCompartmentColour("compartment1", 0xff525252);

  GIVEN("SBML document & geometry image") {
    THEN("find compartments") {
      REQUIRE(s.compartments.size() == 2);
      REQUIRE(s.compartments[0] == "compartment0");
      REQUIRE(s.compartments[1] == "compartment1");
    }
    THEN("find species") {
      REQUIRE(s.species.size() == 2);
      REQUIRE(s.species["compartment0"].size() == 2);
      REQUIRE(s.species["compartment0"][0] == "spec0c0");
      REQUIRE(s.species["compartment0"][1] == "spec1c0");
      REQUIRE(s.species["compartment1"].size() == 3);
      REQUIRE(s.species["compartment1"][0] == "spec0c1");
      REQUIRE(s.species["compartment1"][1] == "spec1c1");
      REQUIRE(s.species["compartment1"][2] == "spec2c1");
    }
    THEN("find reactions") {
      REQUIRE(s.reactions.at("compartment0").size() == 1);
      REQUIRE(s.reactions.at("compartment0")[0] == "reac1");
    }
    WHEN("exportSBMLFile called") {
      THEN(
          "exported file is a SBML level (3,2) document with spatial "
          "extension enabled & required") {
        s.exportSBMLFile("export.xml");
        std::unique_ptr<libsbml::SBMLDocument> doc(
            libsbml::readSBMLFromFile("export.xml"));
        REQUIRE(doc->getLevel() == 3);
        REQUIRE(doc->getVersion() == 2);
        REQUIRE(doc->getPackageRequired("spatial") == true);
        REQUIRE(dynamic_cast<libsbml::SpatialModelPlugin *>(
                    doc->getModel()->getPlugin("spatial")) != nullptr);
      }
    }
    GIVEN("Compartment Colours") {
      QRgb col1 = 0xffaaaaaa;
      QRgb col2 = 0xff525252;
      QRgb col3 = 17423;
      WHEN("compartment colours have been assigned") {
        THEN("can get CompartmentID from colour") {
          REQUIRE(s.getCompartmentID(col1) == "compartment0");
          REQUIRE(s.getCompartmentID(col2) == "compartment1");
          REQUIRE(s.getCompartmentID(col3) == "");
        }
        THEN("can get colour from CompartmentID") {
          REQUIRE(s.getCompartmentColour("compartment0") == col1);
          REQUIRE(s.getCompartmentColour("compartment1") == col2);
        }
      }
      WHEN("new colour assigned") {
        s.setCompartmentColour("compartment0", col1);
        s.setCompartmentColour("compartment1", col2);
        s.setCompartmentColour("compartment0", col3);
        THEN("unassign old colour mapping") {
          REQUIRE(s.getCompartmentID(col1) == "");
          REQUIRE(s.getCompartmentID(col2) == "compartment1");
          REQUIRE(s.getCompartmentID(col3) == "compartment0");
          REQUIRE(s.getCompartmentColour("compartment0") == col3);
          REQUIRE(s.getCompartmentColour("compartment1") == col2);
        }
      }
      WHEN("existing colour re-assigned") {
        s.setCompartmentColour("compartment0", col1);
        s.setCompartmentColour("compartment1", col2);
        s.setCompartmentColour("compartment0", col2);
        THEN("unassign old colour mapping") {
          REQUIRE(s.getCompartmentID(col1) == "");
          REQUIRE(s.getCompartmentID(col2) == "compartment0");
          REQUIRE(s.getCompartmentID(col3) == "");
          REQUIRE(s.getCompartmentColour("compartment0") == col2);
          REQUIRE(s.getCompartmentColour("compartment1") == 0);
        }
      }
    }
  }
}

SCENARIO("import geometry from image", "[sbml][non-gui]") {
  sbml::SbmlDocWrapper s;
  REQUIRE(s.hasGeometry == false);
  GIVEN("Single pixel image") {
    QImage img(1, 1, QImage::Format_RGB32);
    QRgb col = QColor(12, 243, 154).rgba();
    img.setPixel(0, 0, col);
    img.save("tmp.png");
    s.importGeometryFromImage("tmp.png");
    REQUIRE(s.hasGeometry == true);
    THEN("getCompartmentImage returns image") {
      REQUIRE(s.getCompartmentImage().size() == QSize(1, 1));
      REQUIRE(s.getCompartmentImage().pixel(0, 0) == col);
    }
    THEN("image contains no membranes") {
      // todo
    }
  }
}

SCENARIO("SBML test data: ABtoC.xml", "[sbml][non-gui]") {
  sbml::SbmlDocWrapper s;
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());
  GIVEN("SBML document") {
    WHEN("importSBMLFile called") {
      THEN("find compartments") {
        REQUIRE(s.compartments.size() == 1);
        REQUIRE(s.compartments[0] == "comp");
      }
      THEN("find species") {
        REQUIRE(s.species.size() == 1);
        REQUIRE(s.species["comp"].size() == 3);
        REQUIRE(s.species["comp"][0] == "A");
        REQUIRE(s.species["comp"][1] == "B");
        REQUIRE(s.species["comp"][2] == "C");
      }
    }
    WHEN("image geometry imported, assigned to compartment") {
      QImage img(1, 1, QImage::Format_RGB32);
      QRgb col = QColor(12, 243, 154).rgba();
      img.setPixel(0, 0, col);
      img.save("tmp.png");
      s.importGeometryFromImage("tmp.png");
      s.setCompartmentColour("comp", col);
      THEN("getCompartmentImage returns image") {
        REQUIRE(s.getCompartmentImage().size() == QSize(1, 1));
        REQUIRE(s.getCompartmentImage().pixel(0, 0) == col);
      }
      THEN("find membranes") { REQUIRE(s.membraneVec.empty()); }
      THEN("find reactions") {
        REQUIRE(s.reactions.at("comp").size() == 1);
        REQUIRE(s.reactions.at("comp")[0] == "r1");
      }
      THEN("species have default colours") {
        REQUIRE(s.getSpeciesColour("A") == sbml::defaultSpeciesColours()[0]);
        REQUIRE(s.getSpeciesColour("B") == sbml::defaultSpeciesColours()[1]);
        REQUIRE(s.getSpeciesColour("C") == sbml::defaultSpeciesColours()[2]);
      }
      WHEN("species colours changed") {
        QColor newA = QColor(12, 12, 12);
        QColor newB = QColor(123, 321, 1);
        QColor newC = QColor(0, 22, 99);
        s.setSpeciesColour("A", newA);
        s.setSpeciesColour("B", newB);
        s.setSpeciesColour("C", newC);
        REQUIRE(s.getSpeciesColour("A") == newA);
        REQUIRE(s.getSpeciesColour("B") == newB);
        REQUIRE(s.getSpeciesColour("C") == newC);
        s.setSpeciesColour("A", newC);
        REQUIRE(s.getSpeciesColour("A") == newC);
      }
    }
  }
}

SCENARIO("SBML test data: very-simple-model.xml", "[sbml][non-gui]") {
  std::unique_ptr<libsbml::SBMLDocument> doc(
      libsbml::readSBMLFromString(sbml_test_data::very_simple_model().xml));
  // write SBML document to file
  libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");

  sbml::SbmlDocWrapper s;
  s.importSBMLFile("tmp.xml");
  GIVEN("SBML document") {
    WHEN("importSBMLFile called") {
      THEN("find compartments") {
        REQUIRE(s.compartments.size() == 3);
        REQUIRE(s.compartments[0] == "c1");
        REQUIRE(s.compartments[1] == "c2");
        REQUIRE(s.compartments[2] == "c3");
      }
      THEN("find species") {
        REQUIRE(s.species.size() == 3);
        REQUIRE(s.species["c1"].size() == 2);
        REQUIRE(s.species["c1"][0] == "A_c1");
        REQUIRE(s.species["c1"][1] == "B_c1");
        REQUIRE(s.species["c2"].size() == 2);
        REQUIRE(s.species["c2"][0] == "A_c2");
        REQUIRE(s.species["c2"][1] == "B_c2");
        REQUIRE(s.species["c3"].size() == 2);
        REQUIRE(s.species["c3"][0] == "A_c3");
        REQUIRE(s.species["c3"][1] == "B_c3");
      }
    }
  }
}

SCENARIO("SBML test data: yeast-glycolysis.xml", "[sbml][non-gui][inlining]") {
  std::unique_ptr<libsbml::SBMLDocument> doc(
      libsbml::readSBMLFromString(sbml_test_data::yeast_glycolysis().xml));
  // write SBML document to file
  libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");

  sbml::SbmlDocWrapper s;
  s.importSBMLFile("tmp.xml");
  GIVEN("SBML document") {
    WHEN("importSBMLFile called") {
      THEN("find compartments") {
        REQUIRE(s.compartments.size() == 1);
        REQUIRE(s.compartments[0] == "compartment");
      }
      THEN("find species") {
        REQUIRE(s.species.size() == 1);
        REQUIRE(s.species["compartment"].size() == 25);
      }
    }
  }
  GIVEN("inline fn: Glycogen_synthesis_kinetics") {
    std::string expr = "Glycogen_synthesis_kinetics(abc)";
    std::string inlined = "(abc)";
    THEN("return inlined fn") { REQUIRE(s.inlineFunctions(expr) == inlined); }
  }
  GIVEN("inline fn: ATPase_0") {
    std::string expr = "ATPase_0( a,b)";
    std::string inlined = "(b * a)";
    THEN("return inlined fn") { REQUIRE(s.inlineFunctions(expr) == inlined); }
  }
  GIVEN("inline fn: PDC_kinetics") {
    std::string expr = "PDC_kinetics(a,V,k,n)";
    std::string inlined = "(V * (a / k)^n / (1 + (a / k)^n))";
    THEN("return inlined fn") { REQUIRE(s.inlineFunctions(expr) == inlined); }
  }
}