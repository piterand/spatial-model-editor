#include <QtTest>

#include "catch_wrapper.hpp"
#include "dialogimagesize.hpp"
#include "logger.hpp"
#include "qt_test_utils.hpp"
#include "sbml.hpp"

// osx CI tests have issues with key presses & modal dialogs
// for now commenting out this test on osx
#ifndef Q_OS_MACOS
SCENARIO("set size of image", "[dialogimagesize][gui]") {
  GIVEN("100x50 image, initial pixel size 1") {
    sbml::SbmlDocWrapper doc;
    QFile f(":/models/ABtoC.xml");
    f.open(QIODevice::ReadOnly);
    doc.importSBMLString(f.readAll().toStdString());
    auto lengthUnits = doc.modelUnits.length;
    lengthUnits.index = 0;
    REQUIRE(lengthUnits.units[0].scale == 0);
    REQUIRE(lengthUnits.units[1].scale == -1);
    REQUIRE(lengthUnits.units[2].scale == -2);
    REQUIRE(lengthUnits.units[3].scale == -3);
    REQUIRE(lengthUnits.units[4].scale == -6);
    QImage img(100, 50, QImage::Format_ARGB32_Premultiplied);
    img.fill(0xFFFFFFFF);
    DialogImageSize dim(img, 1.0, lengthUnits);
    REQUIRE(dim.getPixelWidth() == dbl_approx(1.0));
    ModalWidgetTimer mwt;
    WHEN("user sets width to 1, same units, pixel size -> 0.01") {
      mwt.setMessage("1");
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(0.01));
    }
    WHEN("user sets width to 1e-8, pixel size -> 1e-10") {
      mwt.setMessage("1e-8");
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(1e-10));
    }
    WHEN("user sets height to 10, pixel size -> 0.2") {
      mwt.setKeySeq({"Tab", "Tab", "1", "0"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(0.2));
    }
    WHEN("user sets height to 10, units to dm, pixel size -> 0.02") {
      mwt.setKeySeq({"Tab", "Down", "Tab", "1", "0"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(0.02));
    }
    WHEN("user sets height to 10, units to cm, pixel size -> 0.002") {
      mwt.setKeySeq({"Tab", "Down", "Down", "Tab", "1", "0"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(0.002));
    }
    WHEN(
        "user sets width to 9 & units to dm, then height 10, units um,"
        " pixel size -> 0.0000002") {
      mwt.setKeySeq(
          {"9", "Tab", "Down", "Tab", "1", "0", "Tab", "Down", "Down", "Down"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(0.0000002));
    }
    WHEN("default units changed to mm, user sets width = 1, units = m") {
      lengthUnits.index = 3;  // mm
      DialogImageSize dim2(img, 22.0, lengthUnits);
      REQUIRE(dim2.getPixelWidth() == dbl_approx(22.0));
      mwt.setKeySeq({"1", "Tab", "Up", "Up", "Up", "Up", "Up", "Up"});
      mwt.start();
      dim2.exec();
      REQUIRE(dim2.getPixelWidth() == dbl_approx(10.0));
    }
  }
}
#endif