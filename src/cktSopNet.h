#ifndef CKT_SOP_NET_H
#define CKT_SOP_NET_H


#include <boost/random.hpp>
#include "cktSop.h"


class Ckt_Sop_t;
class Ckt_Sing_Sel_Info_t;
enum class Ckt_Sop_Cat_t;


class Ckt_Sop_Net_t
{
private:
    abc::Abc_Ntk_t *            pAbcNtk;            // the corresponding ABC network
    int                         nValueClusters;     // nValueClusters = # frame / 64, discard remaining frames
    std::list <Ckt_Sop_t>       cktObjs;            // circuit objects
    std::vector <Ckt_Sop_t *>   pCktPis;            // primary inputs pointers
    std::vector <Ckt_Sop_t *>   pCktPos;            // primary outputs pointers
    Ckt_Sop_t *                 pCktConst0;         // const 0 pointer
    Ckt_Sop_t *                 pCktConst1;         // const 1 pointer

    Ckt_Sop_Net_t &             operator =          (const Ckt_Sop_Net_t & other);

public:
    explicit                    Ckt_Sop_Net_t       (abc::Abc_Ntk_t * p_abc_ntk, int nFrames = 1024);
                                Ckt_Sop_Net_t       (const Ckt_Sop_Net_t & other);
                                ~Ckt_Sop_Net_t      (void);
    void                        PrintInfo           (void) const;
    void                        PrintTopoOrder      (void);
    void                        GenInputDist        (unsigned seed = 314);
    void                        PrintInput          (void) const;
    void                        PrintOutput         (void) const;
    void                        DFS                 (Ckt_Sop_t * pCktObj, std::vector <Ckt_Sop_t *> & pOrderedObjs);
    void                        SetAllUnvisited     (void);
    void                        SetAllUnvisited2    (void);
    void                        SortObjects         (std::vector <Ckt_Sop_t *> & pOrderedObjs);
    void                        FeedForward         (void);
    void                        FeedForward         (std::vector <Ckt_Sop_t *> & pOrderedObjs);
    void                        FeedForward         (std::list <Ckt_Sop_t *> & pOrderedObjs, int i);
    void                        BackupSimRes        (void);
    void                        CheckSimulator      (void);
    void                        TestSimulatorSpeed  (void);
    int                         GetErrorRate        (Ckt_Sop_Net_t & refNtk);
    void                        Replace             (Ckt_Sop_t & cktObj, std::vector <std::string> & newSOP, Ckt_Sop_Cat_t _type, Ckt_Sing_Sel_Info_t & info);
    void                        RecoverFromRpl      (Ckt_Sing_Sel_Info_t & info);
    void                        CheckFanio          (void) const;
    void                        UpdateFoCone        (void);
    void                        PrintCut            (void) const;
    void                        PrintCutNtk         (void) const;
    float                       GetAverNtkSize      (void) const;
    void                        PrintSimRes         (void) const;
    void                        PrintBD             (void) const;

    inline int                  GetObjNum           (void) const        { return static_cast <int> (cktObjs.size()); }
    inline int                  GetPoNum            (void) const        { return static_cast <int> (pCktPos.size()); }
    inline Ckt_Sop_t *          GetPo               (int i = 0) const   { return pCktPos[i]; }
    inline int                  GetValClustersNum   (void) const        { return nValueClusters; }
    inline abc::Abc_Ntk_t *     GetAbcNtk           (void) const        { return pAbcNtk; }
    inline std::string          GetName             (void) const        { return std::string(pAbcNtk->pName); }
};


#endif
