#include "IdrLegacyBridge.h"

#include "IdrAnalysis.h"
#include "IdrImageContext.h"
#include "IdrInstructionNav.h"
#include "IdrLegacyCompat.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <string>

int dummy = 0;
int MaxBufLen = 0;
int DelphiVersion = 0;
MDisasm Disasm;
MKnowledgeBase KnowledgeBase;
DWord CurProcAdr = 0;
DWord ImageSize = 0;
DWord TotalSize = 0;
DWord CodeBase = 0;
DWord CodeSize = 0;
DWord *Flags = nullptr;
PInfoRec *Infos = nullptr;
TStringList *BSSInfos = nullptr;
TList *SegmentList = nullptr;
TList *OwnTypeList = nullptr;
TList *VmtList = nullptr;
char StringBuf[10000] = {};

int cVmtSelfPtr = 0;
int cVmtIntfTable = 0;
int cVmtInitTable = 0;
int cVmtParent = 0;
int cVmtClassName = 0;
int cVmtInstanceSize = 0;

char r80[]="al",r81[]="cl",r82[]="dl",r83[]="bl",r84[]="ah",r85[]="ch",r86[]="dh",r87[]="bh";
char *Reg8Tab[8]={r80,r81,r82,r83,r84,r85,r86,r87};
char r160[]="ax",r161[]="cx",r162[]="dx",r163[]="bx",r164[]="sp",r165[]="bp",r166[]="si",r167[]="di";
char *Reg16Tab[8]={r160,r161,r162,r163,r164,r165,r166,r167};
char r320[]="eax",r321[]="ecx",r322[]="edx",r323[]="ebx",r324[]="esp",r325[]="ebp",r326[]="esi",r327[]="edi";
char *Reg32Tab[8]={r320,r321,r322,r323,r324,r325,r326,r327};
char sr0[]="es",sr1[]="cs",sr2[]="ss",sr3[]="ds",sr4[]="fs",sr5[]="gs",sr6[]="??",sr7[]="??";
char *SegRegTab[8]={sr0,sr1,sr2,sr3,sr4,sr5,sr6,sr7};

namespace {
idr::core::AnalysisState fallbackState;
idr::core::AnalysisState *activeState = &fallbackState;
std::map<String,DWord> classAddressCache;

void SyncLegacyFlagsView(){
    const auto &view=activeState->Flags();
    Flags=view.empty()?nullptr:const_cast<DWord*>(view.data());
}
void EnsureFallbackSize(){
    if(activeState==&fallbackState){
        const auto size=idr::core::GetImageView().size;
        if(fallbackState.Size()!=size) fallbackState.Resize(size);
    }
    SyncLegacyFlagsView();
}
char LowerAscii(char ch){return static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));}
String LowerAsciiCopy(const String &value){String result=value;std::transform(result.begin(),result.end(),result.begin(),LowerAscii);return result;}
}

namespace idr::core {
void SetLegacyAnalysisState(AnalysisState *state){activeState=state?state:&fallbackState;EnsureFallbackSize();}
AnalysisState &LegacyAnalysisState(){EnsureFallbackSize();return *activeState;}
}

bool __fastcall IsFlagSet(DWord flag,int pos){if(pos<0)return false;return idr::core::LegacyAnalysisState().IsFlagSet(flag,static_cast<std::size_t>(pos));}
void __fastcall SetFlag(DWord flag,int pos){if(pos<0)return;idr::core::LegacyAnalysisState().SetFlag(flag,static_cast<std::size_t>(pos));SyncLegacyFlagsView();}
void __fastcall SetFlags(DWord flag,int pos,int num){if(pos<0||num<0)return;idr::core::LegacyAnalysisState().SetFlags(flag,static_cast<std::size_t>(pos),static_cast<std::size_t>(num));SyncLegacyFlagsView();}
void __fastcall ClearFlag(DWord flag,int pos){if(pos<0)return;idr::core::LegacyAnalysisState().ClearFlag(flag,static_cast<std::size_t>(pos));SyncLegacyFlagsView();}
DWord __fastcall Pos2Adr(int pos){if(pos<0)return 0;const auto address=idr::core::OffsetToAddress(static_cast<std::size_t>(pos));return address.value_or(0);}
int __fastcall GetNearestUpInstruction(int pos){return idr::core::GetNearestUpInstruction(idr::core::LegacyAnalysisState(),pos);}
int __fastcall GetNearestUpInstruction(int pos,int toPos){return idr::core::GetNearestUpInstruction(idr::core::LegacyAnalysisState(),pos,toPos);}
String __fastcall ExtractClassName(const String &name){return idr::core::ExtractClassName(name);}
String __fastcall ExtractProcName(const String &name){return idr::core::ExtractProcName(name);}
String __fastcall ExtractName(const String &name){return idr::core::ExtractName(name);}
String __fastcall ExtractType(const String &name){return idr::core::ExtractType(name);}
String __fastcall TrimTypeName(const String &name){return idr::core::TrimTypeName(name);}
String __fastcall GetDefaultProcName(DWord address){return idr::core::DefaultProcName(address);}
String __fastcall MakeGvarName(DWord address){return idr::core::GlobalVarName(address);}

void __fastcall AddClassAdr(DWord address,const String &name){if(!name.empty())classAddressCache[name]=address;}
DWord __fastcall FindClassAdrByName(const String &name){const auto it=classAddressCache.find(name);return it==classAddressCache.end()?0:it->second;}
String PortableWorkDir(){return std::filesystem::current_path().string();}

String __fastcall IntToStr(__int64 value){return std::to_string(value);}
String __fastcall IntToHex(__int64 value,int digits){std::ostringstream out;out<<std::uppercase<<std::hex<<std::setfill('0');if(digits>0)out<<std::setw(digits);out<<static_cast<unsigned long long>(value);return out.str();}
String __fastcall QuotedStr(const String &value){String result;result.reserve(value.size()+2);result.push_back('\'');for(char ch:value){result.push_back(ch);if(ch=='\'')result.push_back('\'');}result.push_back('\'');return result;}
String __fastcall AnsiReplaceText(const String &text,const String &from,const String &to){if(from.empty())return text;String result;String lowerText=LowerAsciiCopy(text);const String lowerFrom=LowerAsciiCopy(from);std::size_t cursor=0;while(cursor<text.size()){const auto found=lowerText.find(lowerFrom,cursor);if(found==String::npos){result.append(text,cursor,String::npos);break;}result.append(text,cursor,found-cursor);result+=to;cursor=found+from.size();}return result;}
bool __fastcall SameText(const String &left,const String &right){return LowerAsciiCopy(left)==LowerAsciiCopy(right);}

template <typename T> String FloatToStr(T value){std::ostringstream out;out<<std::setprecision(std::numeric_limits<T>::max_digits10)<<value;return out.str();}
template String FloatToStr<float>(float);
template String FloatToStr<double>(double);
template String FloatToStr<__int64>(__int64);
template String FloatToStr<long double>(long double);

bool PortableConfirmEmbeddedProcedure(const String &){return false;}
String PortableCurrencyToString(const Currency &value){const bool negative=value.Val<0;const auto magnitude=negative?static_cast<unsigned long long>(-(value.Val+1))+1ULL:static_cast<unsigned long long>(value.Val);const auto whole=magnitude/10000ULL;auto fraction=magnitude%10000ULL;std::ostringstream out;if(negative)out<<'-';out<<whole;if(fraction!=0){out<<'.'<<std::setw(4)<<std::setfill('0')<<fraction;String formatted=out.str();while(!formatted.empty()&&formatted.back()=='0')formatted.pop_back();return formatted;}return out.str();}
