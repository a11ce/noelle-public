#pragma once

#include "noelle/core/AliasAnalysisEngine.hpp"

namespace arcana::noelle {

class ProgramAliasAnalysisEngine : public AliasAnalysisEngine {
public:
  ProgramAliasAnalysisEngine(const std::string &name, void *rawPtr);

  std::string getName(void) const override;

protected:
};

} // namespace arcana::noelle
