namespace sm_starcraft_ai
{
namespace build_inner_states
{
// STATE DECLARATION
struct StiBuild3 : smacc::SmaccState<StiBuild3, SS>
{
  using SmaccState::SmaccState;

// DECLARE CUSTOM OBJECT TAGS
  struct TIMEOUT : SUCCESS{};
  struct NEXT : SUCCESS{};
  struct PREVIOUS : ABORT{};  

// TRANSITION TABLE
  typedef mpl::list<
    
  Transition<EvTimer<CbTimerCountdownOnce, OrTimer>, StiBuild1, TIMEOUT>,  
  Transition<EvKeyPressP<CbDefaultKeyboardBehavior, OrKeyboard>, StiBuild2, PREVIOUS>,
  Transition<EvKeyPressN<CbDefaultKeyboardBehavior, OrKeyboard>, StiBuild1, NEXT>
      
  >reactions;

 // STATE FUNCTIONS
  static void staticConfigure()
  {
    configure_orthogonal<OrTimer, CbTimerCountdownOnce>(10);
    configure_orthogonal<OrSubscriber, CbWatchdogSubscriberBehavior>();
    configure_orthogonal<OrUpdatablePublisher, CbDefaultPublishLoop>();
    configure_orthogonal<OrKeyboard, CbDefaultKeyboardBehavior>();
  }

  void runtimeConfigure()
  {
    // get reference to the client
    ClRosTimer *client;
    this->requiresClient(client);

    // subscribe to the timer client callback
    client->onTimerTick(&StiBuild3::onTimerClientTickCallback, this);

    // getting reference to the single countdown behavior
    auto *cbsingle = this->getOrthogonal<OrTimer>()
                          ->getClientBehavior<CbTimerCountdownOnce>();

    // subscribe to the single countdown behavior callback
    cbsingle->onTimerTick(&StiBuild3::onSingleBehaviorTickCallback, this);
  }

  void onEntry()
  {
    ROS_INFO("On Entry!");
  }

  void onExit()
  {
    ROS_INFO("On Exit!");
  }

  void onTimerClientTickCallback()
  {
    ROS_INFO("timer client tick!");
  }

  void onSingleBehaviorTickCallback()
  {
    ROS_INFO("single behavior tick!");
  }

};
}
}