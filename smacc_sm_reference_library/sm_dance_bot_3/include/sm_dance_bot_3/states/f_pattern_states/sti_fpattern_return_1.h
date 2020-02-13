#pragma once

namespace sm_dance_bot_3
{
namespace f_pattern_states
{
// STATE DECLARATION
template <typename SS>
struct StiFPatternReturn1 : smacc::SmaccState<StiFPatternReturn1<SS>, SS>
{
  typedef SmaccState<StiFPatternReturn1<SS>, SS> TSti;
  using TSti::SmaccState;
  using TSti::context_type;

  // TRANSITION TABLE
  typedef mpl::list<

      Transition<EvActionSucceeded<ClMoveBaseZ, OrNavigation>, StiFPatternRotate1<SS>>

      >
      reactions;

  // STATE FUNCTIONS
  static void staticConfigure()
  {
    TSti::template configure_orthogonal<OrNavigation, CbUndoPathBackwards>();
    TSti::template configure_orthogonal<OrLED, CbLEDOn>();
  }

  void runtimeConfigure()
  {
  }
};
}
}