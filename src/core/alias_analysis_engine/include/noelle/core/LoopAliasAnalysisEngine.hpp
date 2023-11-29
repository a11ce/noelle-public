#pragma once

#include "noelle/core/AliasAnalysisEngine.hpp"

namespace arcana::noelle {

class LoopAliasAnalysisEngine : public AliasAnalysisEngine {
public:
  LoopAliasAnalysisEngine(const std::string &name, void *rawPtr);

  std::string getName(void) const override;

protected:
};

} // namespace arcana::noelle
