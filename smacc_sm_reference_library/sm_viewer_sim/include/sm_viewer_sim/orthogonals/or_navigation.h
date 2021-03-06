#include <smacc/smacc.h>
#include <move_base_z_client_plugin/move_base_z_client_plugin.h>

namespace sm_viewer_sim
{
using namespace cl_move_base_z;

class OrNavigation : public smacc::Orthogonal<OrNavigation>
{
public:
    virtual void onInitialize() override
    {
        auto client = this->createClient<ClMoveBaseZ>();
        client->initialize();
    }
};
} // namespace sm_viewer_sim