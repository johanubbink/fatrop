#ifndef ALGSTRATEGYINCLUDED
#define ALGSTRATEGYINCLUDED
#include "FatropOptions.hpp"
#include <memory>
namespace fatrop
{
    class AlgStrategy
    {
    public:
        AlgStrategy(const std::shared_ptr<FatropOptions> &fatrop_params) : fatrop_params_(fatrop_params){};
        std::shared_ptr<FatropOptions> fatrop_params_;
        void initialize() {};
    };
};
#endif // !ALGSTRATEGYINCLUDED