#include "cktSop.h"


using namespace std;
using namespace abc;


Ckt_Sop_t::Ckt_Sop_t(Abc_Obj_t * p_abc_obj, Ckt_Sop_Net_t * p_ckt_ntk)
    : pAbcObj(p_abc_obj), pCktNtk(p_ckt_ntk), type(Abc_GetSopType(p_abc_obj)), isVisited(false),
    topoId(0), pCktCutNtk(nullptr), pCktObjOri(nullptr), pCktObjCopy(nullptr)
{
    CollectSOP();
    GetLiteralsNum();
    valueClusters.resize(pCktNtk->GetSimNum());
    foConeInfo.resize((Abc_NtkPoNum(pCktNtk->GetAbcNtk()) >> 6) + 1);
    pCktFanins.clear();
    pCktFanouts.clear();
    isDiff.clear();
    BD.clear();
    BDInc.clear();
    BDDec.clear();
}


Ckt_Sop_t::Ckt_Sop_t(Abc_Obj_t * p_abc_obj, Ckt_Sop_Net_t * p_ckt_ntk, Ckt_Sop_Cat_t _type)
    : pAbcObj(p_abc_obj), pCktNtk(p_ckt_ntk), type(_type), isVisited(false),
    topoId(0), pCktCutNtk(nullptr), pCktObjOri(nullptr), pCktObjCopy(nullptr)
{
    CollectSOP();
    GetLiteralsNum();
    valueClusters.resize(pCktNtk->GetSimNum());
    foConeInfo.resize((Abc_NtkPoNum(pCktNtk->GetAbcNtk()) >> 6) + 1);
    pCktFanins.clear();
    pCktFanouts.clear();
    isDiff.clear();
    BD.clear();
    BDInc.clear();
    BDDec.clear();
}


Ckt_Sop_t::Ckt_Sop_t(const Ckt_Sop_t & other)
    : pAbcObj(other.pAbcObj), pCktNtk(other.pCktNtk), type(other.GetType()), isVisited(other.isVisited),
    topoId(other.topoId), pCktCutNtk(other.pCktCutNtk), pCktObjOri(other.pCktObjOri), pCktObjCopy(other.pCktObjCopy)
{
    SOP.assign(other.SOP.begin(), other.SOP.end());
    nLiterals = other.nLiterals;
    valueClusters.resize(other.pCktNtk->GetSimNum());
    foConeInfo.resize(other.foConeInfo.size());
    pCktFanins.clear();
    pCktFanouts.clear();
    isDiff.clear();
    BD.clear();
    BDInc.clear();
    BDDec.clear();
}


Ckt_Sop_t::~Ckt_Sop_t(void)
{
    // free cut network
    ClearCutNtk();
}


void Ckt_Sop_t::PrintFanios(void) const
{
    string temp = "";
    for (auto & pCktFanin : pCktFanins) {
        temp += pCktFanin->GetName();
        temp += ", ";
    }
    cout << setw(30) << setiosflags(ios::left) << temp;
    temp = "";
    for (auto & pCktFanout : pCktFanouts) {
        temp += pCktFanout->GetName();
        temp += ", ";
    }
    cout << setw(30) << setiosflags(ios::left) << temp;
}


void Ckt_Sop_t::CollectSOP(void)
{
    if (IsPI() || IsPO() || IsConst())
        return;
    char * pCube, * pSop = (char *)pAbcObj->pData;
    int Value, v;
    assert(pSop && !Abc_SopIsExorType(pSop));
    int nVars = Abc_SopGetVarNum(pSop);
    assert(!Abc_SopIsComplement(pSop));
    SOP.clear();
    Abc_SopForEachCube(pSop, nVars, pCube) {
        string s = "";
        Abc_CubeForEachVar(pCube, Value, v)
            if (Value == '0' || Value == '1' || Value == '-')
                s += static_cast<char>(Value);
            else
                continue;
        SOP.emplace_back(s);
    }
}


int Ckt_Sop_t::GetLiteralsNum(void)
{
    int ret = 0;
    for (auto & s : SOP)
        for (auto & ch : s)
            if (ch != '-')
                ++ret;
    nLiterals = ret;
    return ret;
}


void Ckt_Sop_t::PrintClusters(void) const
{
    for (auto & cluster : valueClusters) {
        for (int i = 0; i < 64; ++i) {
            cout << Ckt_GetBit(cluster, static_cast <uint64_t> (i));
        }
    }
}


void Ckt_Sop_t::UpdateClusters(void)
{
    switch (type) {
        case Ckt_Sop_Cat_t::PI:
        break;
        case Ckt_Sop_Cat_t::CONST0:
            for (int i = 0; i < static_cast <int> (valueClusters.size()); ++i)
                valueClusters[i] = 0;
        break;
        case Ckt_Sop_Cat_t::CONST1:
            for (int i = 0; i < static_cast <int> (valueClusters.size()); ++i)
                valueClusters[i] = static_cast <uint64_t> (ULLONG_MAX);
        break;
        case Ckt_Sop_Cat_t::PO:
            for (int i = 0; i < static_cast <int> (valueClusters.size()); ++i)
                valueClusters[i] = pCktFanins[0]->valueClusters[i];
        break;
        case Ckt_Sop_Cat_t::INTER:
            for (auto pCube = SOP.begin(); pCube != SOP.end(); ++pCube) {
                vector <uint64_t> product(valueClusters.size(), static_cast <uint64_t> (ULLONG_MAX));
                for (int j = 0; j < static_cast <int> (pCube->length()); ++j) {
                    if ((*pCube)[j] == '0') {
                        for (int i = 0; i < static_cast <int> (valueClusters.size()); ++i)
                            product[i] &= ~(pCktFanins[j]->valueClusters[i]);
                    }
                    else if ((*pCube)[j] == '1') {
                        for (int i = 0; i < static_cast <int> (valueClusters.size()); ++i)
                            product[i] &= pCktFanins[j]->valueClusters[i];
                    }
                }
                if (pCube == SOP.begin())
                    valueClusters.assign(product.begin(), product.end());
                else {
                    for (int i = 0; i < static_cast <int> (valueClusters.size()); ++i)
                        valueClusters[i] |= product[i];
                }
            }
        break;
        default:
        assert(0);
    }
}


void Ckt_Sop_t::FlipClustersFrom(Ckt_Sop_t * pCktObj)
{
    for (int i = 0; i < static_cast <int> (valueClusters.size()); ++i)
        valueClusters[i] = ~pCktObj->valueClusters[i];
}


void Ckt_Sop_t::UpdateCluster(int i)
{
    switch (type) {
        case Ckt_Sop_Cat_t::PI:
        break;
        case Ckt_Sop_Cat_t::CONST0:
            valueClusters[i] = 0;
        break;
        case Ckt_Sop_Cat_t::CONST1:
            valueClusters[i] = static_cast <uint64_t> (ULLONG_MAX);
        break;
        case Ckt_Sop_Cat_t::PO:
            valueClusters[i] = pCktFanins[0]->valueClusters[i];
        break;
        case Ckt_Sop_Cat_t::INTER:
            valueClusters[i] = 0;
            for (auto & cube : SOP) {
                uint64_t product = static_cast <uint64_t> (ULLONG_MAX);
                for (int j = 0; j < static_cast <int> (cube.length()); ++j) {
                    if (cube[j] == '0')
                        product &= ~(pCktFanins[j]->valueClusters[i]);
                    else if (cube[j] == '1')
                        product &= pCktFanins[j]->valueClusters[i];
                }
                valueClusters[i] |= product;
            }
        break;
        default:
        assert(0);
    }
}


uint64_t Ckt_Sop_t::GetClusterValue(vector <string> & newSOP, Ckt_Sop_Cat_t type, int i)
{
    uint64_t ret = 0;
    switch (type) {
        case Ckt_Sop_Cat_t::PI:
        break;
        case Ckt_Sop_Cat_t::CONST0:
            return 0;
        break;
        case Ckt_Sop_Cat_t::CONST1:
            return static_cast <uint64_t> (ULLONG_MAX);
        break;
        case Ckt_Sop_Cat_t::PO:
            return pCktFanins[0]->valueClusters[i];
        break;
        case Ckt_Sop_Cat_t::INTER:
            ret = 0;
            for (auto & cube : newSOP) {
                uint64_t product = static_cast <uint64_t> (ULLONG_MAX);
                for (int j = 0; j < static_cast <int> (cube.length()); ++j) {
                    if (cube[j] == '0')
                        product &= ~(pCktFanins[j]->valueClusters[i]);
                    else if (cube[j] == '1')
                        product &= pCktFanins[j]->valueClusters[i];
                }
                ret |= product;
            }
            return ret;
        break;
        default:
        assert(0);
    }
    return 0;
}


void Ckt_Sop_t::GetClustersValue(vector <string> & newSOP, Ckt_Sop_Cat_t type, vector <uint64_t> & values)
{
    switch (type) {
        case Ckt_Sop_Cat_t::PI:
        break;
        case Ckt_Sop_Cat_t::CONST0:
            for (int i = 0; i < static_cast <int> (values.size()); ++i)
                values[i] = 0;
        break;
        case Ckt_Sop_Cat_t::CONST1:
            for (int i = 0; i < static_cast <int> (values.size()); ++i)
                values[i] = static_cast <uint64_t> (ULLONG_MAX);
        break;
        case Ckt_Sop_Cat_t::PO:
            for (int i = 0; i < static_cast <int> (values.size()); ++i)
                values[i] = pCktFanins[0]->valueClusters[i];
        break;
        case Ckt_Sop_Cat_t::INTER:
            for (auto pCube = newSOP.begin(); pCube != newSOP.end(); ++pCube) {
                vector <uint64_t> product(values.size(), static_cast <uint64_t> (ULLONG_MAX));
                for (int j = 0; j < static_cast <int> (pCube->length()); ++j) {
                    if ((*pCube)[j] == '0') {
                        for (int i = 0; i < static_cast <int> (values.size()); ++i)
                            product[i] &= ~(pCktFanins[j]->valueClusters[i]);
                    }
                    else if ((*pCube)[j] == '1') {
                        for (int i = 0; i < static_cast <int> (values.size()); ++i)
                            product[i] &= pCktFanins[j]->valueClusters[i];
                    }
                }
                if (pCube == newSOP.begin())
                    values.assign(product.begin(), product.end());
                else {
                    for (int i = 0; i < static_cast <int> (values.size()); ++i)
                        values[i] |= product[i];
                }
            }
            break;
        default:
            assert(0);
    }
}


void Ckt_Sop_t::XorClustersValue(vector <uint64_t> & values)
{
    for (int i = 0; i < static_cast <int> (values.size()); ++i)
        values[i] ^= valueClusters[i];
}


void Ckt_Sop_t::ReplaceBy(vector <string> & newSOP, Ckt_Sop_Cat_t _type, Ckt_Sing_Sel_Info_t & info)
{
    // backup
    info.pCktObj = this;
    info.type = type;
    info.SOP.assign(SOP.begin(), SOP.end());
    // local approximate change
    SOP.assign(newSOP.begin(), newSOP.end());
    type = _type;
    // change SOP of ABC object
    string tmp("");
    if (type == Ckt_Sop_Cat_t::INTER) {
        for (auto & cube : newSOP) {
            tmp += cube;
            tmp += " 1\n";
        }
        tmp += "\0";
    }
    else if (type == Ckt_Sop_Cat_t::CONST0) {
        tmp = "";
        for (int i = 0; i < GetFaninNum(); ++i)
            tmp += "-";
        tmp += " 0\n\0";
    }
    else if (type == Ckt_Sop_Cat_t::CONST1) {
        tmp = "";
        for (int i = 0; i < GetFaninNum(); ++i)
            tmp += "-";
        tmp += " 1\n\0";
    }
    else
        assert(0);
    memcpy(pAbcObj->pData, tmp.c_str(), tmp.length() + 1);
}


void Ckt_Sop_t::CheckFanio(void) const
{
    Abc_Obj_t * pObj;
    int i;
    assert(Abc_ObjFaninNum(pAbcObj) == static_cast <int> (pCktFanins.size()));
    assert(Abc_ObjFanoutNum(pAbcObj) == static_cast <int> (pCktFanouts.size()));
    Abc_ObjForEachFanout(pAbcObj, pObj, i)
        assert(static_cast <string> (Abc_ObjName(pObj)) == pCktFanouts[i]->GetName());
    Abc_ObjForEachFanin(pAbcObj, pObj, i)
        assert(static_cast <string> (Abc_ObjName(pObj)) == pCktFanins[i]->GetName());
}


void Ckt_Sop_t::ClearCutNtk(void)
{
    if (pCktCutNtk != nullptr) {
        delete pCktCutNtk;
        pCktCutNtk = nullptr;
    }
}


void Ckt_Sop_t::ClearMem(void)
{
    vector <string>().swap(SOP);
    vector <uint64_t>().swap(valueClusters);
    vector <uint64_t>().swap(foConeInfo);
    vector <Ckt_Sop_t *>().swap(pCktFanins);
    vector <Ckt_Sop_t *>().swap(pCktFanouts);
    vector <uint64_t>().swap(isDiff);
    vector <uint64_t>().swap(BD);
    vector <uint64_t>().swap(BDInc);
    vector <uint64_t>().swap(BDDec);
}


Ckt_Sing_Sel_Info_t::Ckt_Sing_Sel_Info_t(void)
    : pCktObj(nullptr), type(Ckt_Sop_Cat_t::INTER)
{
    SOP.clear();
}


Ckt_Sing_Sel_Info_t::Ckt_Sing_Sel_Info_t(Ckt_Sop_t * p_ckt_obj, Ckt_Sop_Cat_t _type, const vector <string> & _sop)
    : pCktObj(p_ckt_obj), type(_type)
{
    SOP.assign(_sop.begin(), _sop.end());
}

Ckt_Sing_Sel_Info_t::~Ckt_Sing_Sel_Info_t(void)
{
}


ostream & operator << (ostream & os, const vector <string> & SOP)
{
    for (auto & s : SOP)
        cout << s << "|";
    return os;
}


ostream & operator << (ostream & os, const Ckt_Sop_Cat_t & type)
{
    switch ( type ) {
        case Ckt_Sop_Cat_t::PI:
            cout << "PI";
        break;
        case Ckt_Sop_Cat_t::PO:
            cout << "PO";
        break;
        case Ckt_Sop_Cat_t::CONST0:
            cout << "CONST0";
        break;
        case Ckt_Sop_Cat_t::CONST1:
            cout << "CONST1";
        break;
        case Ckt_Sop_Cat_t::INTER:
            cout << "INTER";
        break;
        default:
            assert(0);
    }
    return os;
}


Ckt_Sop_Cat_t Abc_GetSopType( Abc_Obj_t * pObj )
{
    if (Abc_ObjIsPi(pObj))
        return Ckt_Sop_Cat_t::PI;
    if (Abc_ObjIsPo(pObj))
        return Ckt_Sop_Cat_t::PO;
    assert(Abc_ObjIsNode(pObj));
    if ( Abc_NodeIsConst0(pObj) )
        return Ckt_Sop_Cat_t::CONST0;
    else if ( Abc_NodeIsConst1(pObj) )
        return Ckt_Sop_Cat_t::CONST1;
    else
        return Ckt_Sop_Cat_t::INTER;
}


int GetLiteralsNum(const vector <string> & SOP)
{
    int ret = 0;
    for (auto & s : SOP)
        for (auto & ch : s)
            if (ch != '-')
                ++ret;
    return ret;
}
