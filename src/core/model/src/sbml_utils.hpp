#pragma once

#include <optional>
#include <string>
#include <utility>

namespace libsbml {
class Geometry;
class Model;
class SampledFieldGeometry;
class Species;
}  // namespace libsbml

libsbml::SampledFieldGeometry *getSampledFieldGeometry(libsbml::Geometry *geom);
const libsbml::Geometry *getOrCreateGeometry(const libsbml::Model *model);
libsbml::Geometry *getOrCreateGeometry(libsbml::Model *model);
void createDefaultCompartmentGeometryIfMissing(libsbml::Model *model);

unsigned int getNumSpatialDimensions(const libsbml::Model *model);

std::string getDomainIdFromCompartmentId(const libsbml::Model *model,
                                         const std::string &compartmentId);

std::optional<std::pair<std::string, std::string>> getAdjacentCompartments(
    const libsbml::Model *model, const std::string &compartmentId);

bool getIsSpeciesConstant(const libsbml::Species *spec);