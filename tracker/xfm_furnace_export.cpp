// Reusable XFM song -> Furnace YM2612 encoder.
#include "xfm_furnace_export.h"
#include "xfm_api.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
using std::string;
using Bytes = std::vector<uint8_t>;
static constexpr int version = 244;
static void check(bool ok, const string& why) { if (!ok) throw std::runtime_error(why); }
static void b(Bytes& out, int n) { check(n >= 0 && n <= 255, "byte out of range"); out.push_back(uint8_t(n)); }
static void w(Bytes& out, int n) { check(n >= 0 && n <= 65535, "word out of range"); b(out,n&255); b(out,(n>>8)&255); }
static void d(Bytes& out, uint32_t n) { for (int i=0;i<4;i++) b(out,(n>>(i*8))&255); }
static void f(Bytes& out, float n) { uint32_t bits; std::memcpy(&bits,&n,4); d(out,bits); }
static void str(Bytes& out, const string& s) { out.insert(out.end(),s.begin(),s.end()); b(out,0); }
static void cat(Bytes& out, const Bytes& more) { out.insert(out.end(),more.begin(),more.end()); }
static Bytes block(const string& code, const Bytes& body) { Bytes out; out.insert(out.end(),code.begin(),code.end()); d(out,body.size()); cat(out,body); return out; }
static Bytes feature(const string& code, const Bytes& body) { Bytes out; out.insert(out.end(),code.begin(),code.end()); w(out,body.size()); cat(out,body); return out; }
static string trim(string s) { auto a=s.find_first_not_of(" \t\r\n"); if(a==string::npos)return {}; auto z=s.find_last_not_of(" \t\r\n"); return s.substr(a,z-a+1); }
static std::vector<string> split(const string& s, char sep) { std::vector<string> v; size_t a=0; for(;;){auto z=s.find(sep,a);v.push_back(s.substr(a,z==string::npos?z:z-a));if(z==string::npos)break;a=z+1;}return v; }
static string raw(const string& text,const string& macro,const string& delimiter) {
    auto p=text.find(macro+"(R\""+delimiter+"("); check(p!=string::npos,"missing "+macro); p+=macro.size()+4+delimiter.size();
    auto z=text.find(")"+delimiter+"\")",p); check(z!=string::npos,"unterminated "+macro); return text.substr(p,z-p);
}
static int setting(const string& text,const string& name,int fallback=-1) {
    auto p=text.find(name+"("); if(p==string::npos){check(fallback>=0,"missing "+name);return fallback;} p+=name.size()+1;
    auto z=text.find(')',p);check(z!=string::npos,"invalid "+name);return std::stoi(text.substr(p,z-p));
}
static int hex(const string& s) { check(!s.empty(),"empty hex");try{return std::stoi(s,nullptr,16);}catch(...){throw std::runtime_error("invalid hex: '"+s+"'");} }
static string hex2(int value) { const char* digits="0123456789ABCDEF";return string{digits[(value>>4)&15],digits[value&15]}; }
static std::vector<int> ints(const string& s) { std::istringstream in(s); std::vector<int> values; int x; while(in>>x)values.push_back(x);return values; }
struct Macro { int target,loop,release;std::vector<int> values; };
struct Instrument { int number;string name;std::array<int,4> patch{};std::array<std::array<int,11>,4> ops{};int op_count=0;std::vector<Macro> macros; };
struct Cell { int note=-1,ins=-1,volume=-1;std::vector<std::pair<int,int>> effects;string original; };
using Row=std::array<Cell,6>;
struct Song { string name;int tick_rate,speed,rows_per_beat,tuning,lfo_on,lfo_freq;std::vector<Row> rows;std::map<int,string> sections;std::vector<Instrument> instruments; };
struct Part { int start;string name;std::vector<Row> rows; };
static Cell parse_cell(const string& text) {
    check(text.size()==7 || text.size()==11 || text.size()==15,"invalid cell: "+text);
    Cell c;c.original=text;auto n=text.substr(0,3);
    if(n=="OFF") c.note=180; else if(n=="REL") c.note=181; else if(n=="===") c.note=182;
    else if(n!="...") {
        static const std::array<string,12> notes={"C-","C#","D-","D#","E-","F-","F#","G-","G#","A-","A#","B-"};
        auto it=std::find(notes.begin(),notes.end(),n.substr(0,2));check(it!=notes.end() && n[2]>='0' && n[2]<='9',"invalid note: "+n);
        c.note=60+(n[2]-'0')*12+int(it-notes.begin());check(c.note<180,"note outside Furnace range");
    }
    if(text.substr(3,2)!="..")c.ins=hex(text.substr(3,2));
    if(text.substr(5,2)!="..")c.volume=hex(text.substr(5,2));
    for(size_t i=7;i<text.size();i+=4)if(text.substr(i,4)!="....")c.effects.emplace_back(hex(text.substr(i,2)),hex(text.substr(i+2,2)));
    return c;
}
static Song parse_song(const string& text) {
    Song s; s.name=trim(raw(text,"XFM_SONG_BEGIN","xfmname"));s.tick_rate=setting(text,"XFM_TICK_RATE");s.speed=setting(text,"XFM_SPEED");
    s.rows_per_beat=setting(text,"XFM_ROWS_PER_BEAT",4);s.tuning=setting(text,"XFM_TUNING_MODE",0);
    s.lfo_on=setting(text,"XFM_LFO_ENABLED",1);s.lfo_freq=setting(text,"XFM_LFO_FREQUENCY",0);
    std::istringstream pattern(raw(text,"XFM_PATTERN","xfmpattern"));string line;
    while(std::getline(pattern,line) && trim(line).empty()) {}
    check(!trim(line).empty(),"empty pattern");int declared=std::stoi(trim(line));
    while(std::getline(pattern,line)) {
        line=trim(line);if(line.empty() || line=="COLLAPSED" || line.rfind("PART_COLOR ",0)==0)continue;
        if(line.rfind("PART ",0)==0 || line.rfind("SKIP ",0)==0){s.sections[int(s.rows.size())]=line;continue;}
        auto cells=split(line,'|');check(cells.size()==6,"row must have six cells");Row row;
        for(int ch=0;ch<6;ch++)row[ch]=parse_cell(cells[ch]);s.rows.push_back(std::move(row));
    }
    check(int(s.rows.size())==declared,"pattern row count mismatch");
    std::istringstream instruments(raw(text,"XFM_INSTRUMENTS","xfminstruments"));Instrument current{};bool inside=false;
    while(std::getline(instruments,line)) {
        line=trim(line);if(line.empty() || line.rfind("//",0)==0)continue;
        std::istringstream in(line);string tag;in>>tag;
        if(tag=="INST") {check(!inside,"nested INST");string n;in>>n;current=Instrument{};current.number=hex(n);inside=true;}
        else if(tag=="ENDINST") {check(inside && current.op_count==4,"incomplete instrument");s.instruments.push_back(current);inside=false;}
        else if(tag=="NAME") {check(inside,"NAME outside instrument");current.name=trim(line.substr(5));}
        else if(tag=="PATCH") {check(inside,"PATCH outside instrument");auto v=ints(line.substr(6));check(v.size()==4,"bad PATCH");std::copy(v.begin(),v.end(),current.patch.begin());}
        else if(tag=="FM" || tag=="OP") {
            check(inside,"operator outside instrument");string op;in>>op;if(op=="OP")continue;
            int idx=std::stoi(op);check(idx>=1 && idx<=4,"bad operator number");auto v=ints(line.substr(line.find(op)+op.size()));check(v.size()==11,"bad operator");
            if(tag=="FM") { // TL AR DR SL SR RR SSG MUL DT RS AM
                std::copy(v.begin(),v.end(),current.ops[idx-1].begin());
            } else { // DT MUL TL RS AR AM DR SR SL RR SSG
                current.ops[idx-1]={v[2],v[4],v[6],v[8],v[7],v[9],v[10],v[1],v[0],v[3],v[5]};
            }
            current.op_count++;
        }
        else if(tag=="MACRO") {
            check(inside,"MACRO outside instrument");auto v=ints(line.substr(6));check(v.size()>=5 && v[1]>=1 && v[1]<=64 && int(v.size())==v[1]+4,"bad MACRO");
            current.macros.push_back({v[0],v[2],v[3],{v.begin()+4,v.end()}});
        }
        else if(tag!="COLOR" && tag!="ALG")throw std::runtime_error("unknown instrument line: "+line);
    }
    check(!inside,"missing ENDINST");return s;
}
static std::vector<Part> split_parts(const Song& s) {
    std::vector<int> bounds={0,int(s.rows.size())};for(auto& [start,name]:s.sections)bounds.push_back(start);
    std::sort(bounds.begin(),bounds.end());bounds.erase(std::unique(bounds.begin(),bounds.end()),bounds.end());std::vector<Part> parts;
    for(size_t i=1;i<bounds.size();i++){int start=bounds[i-1],end=bounds[i];auto it=s.sections.find(start);
        string name=it==s.sections.end()?"PART "+std::to_string(start):it->second;if(name.rfind("SKIP ",0)==0)continue;
        for(int offset=start;offset<end;offset+=256){Part p{offset,name+(offset==start?"":" +"+std::to_string(offset-start)),{}};
            p.rows.insert(p.rows.end(),s.rows.begin()+offset,s.rows.begin()+std::min(offset+256,end));parts.push_back(std::move(p));}
    }
    check(!parts.empty() && parts.size()<=256,"invalid order count");return parts;
}
static std::vector<int> engine_volumes(const Song& song,const std::vector<Part>& parts) {
    string pattern;int count=0;for(auto& part:parts)count+=int(part.rows.size());pattern=std::to_string(count)+"\n";
    for(auto& part:parts)for(auto& row:part.rows){for(int ch=0;ch<6;ch++){if(ch)pattern+='|';pattern+=row[ch].original;}pattern+='\n';}
    using Module=std::unique_ptr<xfm_module,decltype(&xfm_module_destroy)>;
    Module module(xfm_module_create(44100,256,XFM_CHIP_YM2612),&xfm_module_destroy);check(bool(module),"cannot create XFM module");
    check(xfm_song_declare(module.get(),1,pattern.c_str(),song.tick_rate,song.speed)>0,"XFM song parser rejected pattern");
    xfm_song_play(module.get(),1,false);auto& active=module->active_song;int row_samples=module->song_patterns[1].samples_per_row;
    check(row_samples>=2,"row too short");std::array<int,6> prior{};prior.fill(127);std::vector<int> levels;levels.reserve(count*6);
    std::array<int16_t,512> audio{};auto advance=[&](int frames){while(frames){int n=std::min(frames,256);xfm_mix_song(module.get(),audio.data(),n);frames-=n;}};
    int row_index=0;
    for(auto& part:parts)for(auto& row:part.rows){
        check(active.active && active.current_row==row_index && active.sample_in_row==0,"XFM row position changed unexpectedly");
        for(int ch=0;ch<6;ch++)levels.push_back(row[ch].volume>=0?row[ch].volume:prior[ch]);
        advance(row_samples-1);check(active.current_row==row_index,"XFM row ended early");
        for(int ch=0;ch<6;ch++)prior[ch]=active.channels[ch].current_volume;
        advance(1);row_index++;
    }
    return levels;
}
static std::pair<string,int> macro_target(int t) {
    if(t>=1 && t<=4)return {"O"+std::to_string(t),6};
    if(t>=5 && t<=8)return {"O"+std::to_string(t-4),3};
    if(t>=9 && t<=12)return {"O"+std::to_string(t-8),9};
    if(t==13)return {"MA",9};if(t==14)return {"MA",1};
    for(auto [start,code]:std::array<std::pair<int,int>,6>{{{19,1},{23,2},{27,10},{31,5},{35,4},{39,11}}})
        if(t>=start && t<start+4)return {"O"+std::to_string(t-start+1),code};
    throw std::runtime_error("unsupported macro target "+std::to_string(t));
}
static int detune(int dt) {switch(dt){case -3:return 2;case -2:return 1;case -1:return 0;case 0:return 3;case 1:return 4;case 2:return 5;case 3:return 6;}throw std::runtime_error("invalid detune");}
static Bytes encode_macro(const Macro& m,int code,bool is_dt) {
    std::vector<int> v=m.values;if(is_dt)for(auto& x:v)x=detune(x);
    auto [lo,hi]=std::minmax_element(v.begin(),v.end());int wordsize=0;
    if(*lo>=0 && *hi<=255)wordsize=0;else if(*lo>=-128 && *hi<=127)wordsize=64;
    else if(*lo>=-32768 && *hi<=32767)wordsize=128;else wordsize=192;
    Bytes out;for(int x:{code,int(v.size()),m.loop,m.release,0,wordsize,0,1})b(out,x);
    for(int x:v){if(wordsize==0 || wordsize==64)b(out,x&255);else if(wordsize==128)w(out,x&65535);else d(out,uint32_t(x));}return out;
}
static Bytes encode_instrument(const Instrument& ins) {
    auto [alg,fb,ams,fms]=ins.patch;Bytes fm;
    for(int x:{0xF4,(alg<<4)|fb,(ams<<3)|fms,0x20,0})b(fm,x);
    for(auto& o:ins.ops){auto [tl,ar,dr,sl,sr,rr,ssg,mul,dt,rs,am]=o;
        for(int x:{(detune(dt)<<4)|mul,tl,(rs<<6)|ar,(am<<7)|dr,sr,(sl<<4)|rr,ssg,0})b(fm,x);
    }
    Bytes name;str(name,ins.name.empty()?"Instrument "+hex2(ins.number):ins.name);
    Bytes features=feature("NA",name);cat(features,feature("FM",fm));std::map<string,Bytes> groups;std::vector<string> order;
    for(auto& macro:ins.macros){auto [group,code]=macro_target(macro.target);if(!groups.count(group)){Bytes data;w(data,8);groups[group]=data;order.push_back(group);}
        cat(groups[group],encode_macro(macro,code,macro.target>=9 && macro.target<=12));}
    for(auto& group:order){b(groups[group],255);cat(features,feature(group,groups[group]));}
    cat(features,Bytes{'E','N',0,0});Bytes body;w(body,version);b(body,1);b(body,0);cat(body,features);return block("INS2",body);
}
static Bytes encode_cell(const Cell& cell,const std::map<int,int>& remap) {
    Bytes payload;int mask=0,effect_mask=0;check(cell.effects.size()<=4,"too many effects");
    if(cell.note>=0){mask|=1;b(payload,cell.note);}
    if(cell.ins>=0){auto it=remap.find(cell.ins);check(it!=remap.end(),"undefined instrument");mask|=2;b(payload,it->second);}
    if(cell.volume>=0){mask|=4;b(payload,cell.volume);}
    for(size_t i=0;i<cell.effects.size();i++){if(i==0)mask|=0x18;else{mask|=0x20;effect_mask|=3<<(i*2);}b(payload,cell.effects[i].first);b(payload,cell.effects[i].second);}
    Bytes out;b(out,mask);if(cell.effects.size()>1)b(out,effect_mask);cat(out,payload);return out;
}
static Bytes encode_pattern(int ch,int order,const Part& part,const std::map<int,int>& remap) {
    Bytes payload;int empty=0;
    for(auto& row:part.rows){auto& cell=row[ch];if(cell.note<0 && cell.ins<0 && cell.volume<0 && cell.effects.empty()){
        empty++;if(empty==129){b(payload,0xfe);empty=1;}continue;}
        while(empty>=2){int count=std::min(empty,128);b(payload,0x80|(count-2));empty-=count;}
        if(empty){b(payload,0);empty=0;}cat(payload,encode_cell(cell,remap));
    }
    b(payload,255);Bytes body;b(body,0);b(body,ch);w(body,order);str(body,part.name);cat(body,payload);return block("PATN",body);
}
static Bytes encode_song(const Song& s,int length,int orders,const std::array<int,6>& effect_cols) {
    Bytes body;f(body,float(s.tick_rate));b(body,1);b(body,1);w(body,length);w(body,orders);b(body,s.rows_per_beat);b(body,16);w(body,150);w(body,150);
    b(body,1);w(body,s.speed);body.insert(body.end(),30,0);str(body,s.name);b(body,0);
    for(int ch=0;ch<6;ch++)for(int order=0;order<orders;order++)b(body,order);
    for(int n:effect_cols)b(body,n);body.insert(body.end(),6,3);body.insert(body.end(),6,0);body.insert(body.end(),12,0);body.insert(body.end(),24,0);
    return block("SNG2",body);
}
static Bytes convert(Song& song) {
    check(song.tuning==0,"just intonation cannot be exported exactly");check(song.lfo_on>=0 && song.lfo_on<=1 && song.lfo_freq>=0 && song.lfo_freq<=7,"invalid LFO");
    auto parts=split_parts(song);auto levels=engine_volumes(song,parts);int row_index=0;
    for(auto& part:parts)for(auto& row:part.rows){for(int ch=0;ch<6;ch++)if(row[ch].note>=0 && row[ch].note<180 && row[ch].volume<0)row[ch].volume=levels[row_index*6+ch];row_index++;}
    int length=0;for(auto& part:parts)length=std::max(length,int(part.rows.size()));
    for(size_t i=0;i<parts.size();i++)if(int(parts[i].rows.size())<length){Row nav;nav[0].effects.push_back({i+1==parts.size()?0x0b:0x0d,0});parts[i].rows.push_back(nav);}
    auto& first=parts[0].rows[0][0];first.effects.push_back({0x10,(song.lfo_on<<4)|song.lfo_freq});std::array<int,6> cols{int(std::max<size_t>(2,first.effects.size())),2,2,2,2,2};
    std::map<int,int> remap;for(size_t i=0;i<song.instruments.size();i++)remap[song.instruments[i].number]=int(i);
    std::vector<Bytes> ins,patterns;for(auto& instrument:song.instruments)ins.push_back(encode_instrument(instrument));
    for(int ch=0;ch<6;ch++)for(size_t order=0;order<parts.size();order++)patterns.push_back(encode_pattern(ch,int(order),parts[order],remap));
    Bytes head;for(char x:string("-Furnace module-"))b(head,x);w(head,version);w(head,0);d(head,32);head.insert(head.end(),8,0);
    Bytes body;str(body,song.name);body.insert(body.end(),7,0);f(body,440);b(body,1);f(body,1);w(body,6);w(body,1);w(body,0x83);w(body,6);f(body,1);f(body,0);f(body,0);d(body,0);b(body,1);
    std::vector<std::vector<size_t>> positions;auto group=[&](int kind,int count){b(body,kind);d(body,count);std::vector<size_t> p;for(int i=0;i<count;i++){p.push_back(body.size());d(body,0);}positions.push_back(p);};
    group(1,1);group(4,int(ins.size()));group(7,int(patterns.size()));b(body,0);
    Bytes out=head;cat(out,block("INF2",body));std::vector<std::vector<Bytes>> groups={{encode_song(song,length,int(parts.size()),cols)},ins,patterns};
    for(size_t g=0;g<groups.size();g++)for(size_t i=0;i<groups[g].size();i++){uint32_t offset=uint32_t(out.size());size_t pos=40+positions[g][i];for(int z=0;z<4;z++)out[pos+z]=uint8_t(offset>>(z*8));cat(out,groups[g][i]);}
    return out;
}

XfmFurnaceExportResult xfm_export_furnace_from_header(const std::string& source) {
    Song song = parse_song(source);
    XfmFurnaceExportResult result;
    result.source_rows = song.rows.size();
    result.instrument_count = song.instruments.size();
    result.data = convert(song);
    return result;
}
