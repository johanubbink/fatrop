// solver data
#ifndef FATROPDATAINCLUDED
#define FATROPDATAINCLUDED
#include "BLASFEO_WRAPPER/LinearAlgebraBlasfeo.hpp"
#include "TEMPLATES/NLPAlg.hpp"
using namespace std;
namespace fatrop
{
    struct FatropData : public RefCountedObj
    {
        FatropData(const NLPDims &nlpdims) : nlpdims(nlpdims),
                                             memvars(nlpdims.nvars, 6),
                                             memeqs(nlpdims.neqs, 4),
                                             x_curr(memvars[0]),
                                             x_next(memvars[1]),
                                             delta_x(memvars[2]),
                                             x_scales(memvars[3]),
                                             lam_curr(memeqs[0]),
                                             lam_next(memeqs[1]),
                                             lam_scales(memeqs[2]),
                                             g_curr(memeqs[3]),
                                             g_next(memeqs[4]),
                                             grad_curr(memvars[4]),
                                             grad_next(memvars[5])

        {
        }
        const NLPDims nlpdims;
        double obj_scale = 1.0;
        FatropMemoryVecBF memvars;
        FatropMemoryVecBF memeqs;
        FatropVecBF x_curr;
        FatropVecBF x_next;
        FatropVecBF delta_x;
        FatropVecBF x_scales;
        FatropVecBF lam_curr;
        FatropVecBF lam_next;
        FatropVecBF lam_scales;
        FatropVecBF g_curr;
        FatropVecBF g_next;
        FatropVecBF grad_curr;
        FatropVecBF grad_next;
    };
}
#endif // FATROPDATAINCLUDED