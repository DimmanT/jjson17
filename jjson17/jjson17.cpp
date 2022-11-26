#include "jjson17.h"

#include <sstream>
#include <iomanip>

#ifdef JJSON17_PARSE
#include <array>
#include <algorithm>
#endif

#include <iostream>

using namespace std;

namespace jjson17 {

//---------- блок Value ---------------
static constexpr Type type_of(const Value_t& value)
{
    return static_cast<Type>(value.index());    //следует следить за порядком типов в объявлении варианта
}

/**
 * \details Если значение \c *this не является массивом, то будет выброшено исключение типа \c logic_error .
 */
void Value::push_back(Value v)
{
    Type t = type_of(*this);
    if(t == Type::ARRAY)
         get<Array>(*this).push_back(v);
    else throw logic_error("This value IS NOT an ARRAY. Rebuilding into array is not supported yet.");
}

/**
 * \details Если значение \c *this не является объектом, то будет выброшено исключение типа \c logic_error .
 */
void Value::insert(Record r)
{
    Type t = type_of(*this);
    if(t == Type::OBJECT)
         get<Object>(*this).insert(r);
    else throw logic_error("This value IS NOT an OBJECT. Rebuilding into object is not supported yet.");
}

bool Value::isNull()
{
    return type_of(*this) == Type::NUL;
}

Value::operator double() const
{
    switch (type_of(*this)) {
        case Type::REAL   : return get<double> (*this);
        case Type::INTEGER: return static_cast<double>(get<int64_t>(*this));
        default: throw bad_variant_access();
    }
}
Value::operator string() const  { return get<string>(*this);}
Value::operator bool()   const  { return get<bool  >(*this);}
Value::operator Array()  const  { return get<Array >(*this);}
Value::operator Object() const  { return get<Object>(*this);}
//--------------------------------------

//----------- блок Вывода --------------
enum class SkipTag : bool {NO,YES};

static void writeValue(ostream& s,uint16_t depth, const string& tag,const Value_t& val, SkipTag skipTag);

/**
 * @brief escape
 * @param str
 * @return строку в UTF-8 с экранированными символами согласно рекомендации \link json.org
 */
static string escape(string&& str)
{
    //предполагается, что строка utf8
    //таблица размера символа utf8 в байтах по префиксу первого байта
    static const uint8_t SIZE_CODE[] {
        1,1,1,1,1,1,1,1,  //0000-0111 - 1 byte
        1,1,1,1,          //1000-1011 - недопустимые
        2,2,              //1100-1101 - 2 byte
        3,                //1110      - 3 byte
        4                 //1111      - 4 byte
    };

    size_t i = 0;
    while(i < str.size())
    {
        auto mv = SIZE_CODE[ (str[i]&0xF0) >> 4 ]; //определяем сколько байт в символе UTF-8.
        if(mv == 1) {
            switch (str[i]) {
                case '"'  :
                case '\\' :
                case '/'  : str.insert(i,1,'\\'); i++; break;
                case '\b' : str[i] = 'b';   str.insert(i,1,'\\'); i++; break;
                case '\f' : str[i] = 'f';   str.insert(i,1,'\\'); i++; break;
                case '\n' : str[i] = 'n';   str.insert(i,1,'\\'); i++; break;
                case '\r' : str[i] = 'r';   str.insert(i,1,'\\'); i++; break;
                case '\t' : str[i] = 't';   str.insert(i,1,'\\'); i++; break;
                default   : break;
            }
        }
        i+=mv;  // сдвигаемся на количество байт соотв. размеру UTF-8 символа
    }

    return move(str);
}
static auto prepareStr(string str)
{
    return '"'+escape(move(str))+'"';
}

struct Visitor {
    Visitor(ostream& stream, uint16_t depth, bool skipTag) : s(stream),depth(depth),skipTag(skipTag) {}
    void operator()(nullptr_t             ) {s << "null";}
    void operator()(bool               val) {s << val;}
    void operator()(const string& str) {s << prepareStr(str);}
    void operator()(int64_t            num) {s << num;}
    void operator()(double             num) {s << num;}
    void operator()(const Array&       arr)
    {
        if(!skipTag) {
            s << endl;
            insertTabs();
        }
        s << '[' ;
        if(!arr.empty())
        {
            s<<endl;
            depth++;
            for(size_t i = 0; i < arr.size()-1; ++i)
            {
                insertTabs();
                writeValue(s,depth,"",arr[i],SkipTag::YES);
                s<<','<<endl;
            }
            insertTabs();                                    // |
            writeValue(s,depth,"",arr.back(),SkipTag::YES);  // |>  то же, но без ','
            s<<endl;                                         // |
            depth--;
            insertTabs();
        }
        s << ']';
    }
    void operator()(const Object&      obj)
    {
        if(!skipTag)  {
            s << endl;
            insertTabs();
        }
        s<<'{';
        if(!obj.empty())
        {
            s << endl;
            depth++;
            auto last = obj.end();last--;
            for(const auto& [k,v] : obj)
            {
                insertTabs();
                writeValue(s,depth,k,v,SkipTag::NO);
                if(k!=last->first) s<<',';
                s<<endl;
            }
            depth--;
            insertTabs();
        }
        s<<'}';
    }
private:
    ostream& s;
    uint16_t      depth;
    bool          skipTag;

    inline void insertTabs()  {
        for(int t = 0; t < depth; ++t)  s<<'\t';
    }
};

/**
 * @brief writeValue рекурсивная функция, заполняющая поток \c s значением \c val и вызываемая для каждого подзначения, хранимого в \c val.
 * @param s         поток, в который будут записываться значения.
 * @param depth     отражает глубину вложенности, определяет число отступов ('tab') вставляемых перед значением.
 * @param tag       наименование значения. Можно оставить пустым и выставить параметр \c skipTag в \c SkipTag::YES, тогда наименование будет опущено и символ ':' не будет напечатан.
 * @param v         само значение
 * @param skipTag   если выставлен в \c SkipTag::YES , то в поток будет записано только значение без, наименования и символа ':'
 */
static void writeValue(ostream& s,uint16_t depth, const string& tag,const Value_t& val, SkipTag skipTag)
{
    if(skipTag == SkipTag::NO)
        s << prepareStr(tag)<<':'<<'\t';
    visit(Visitor{s, depth, skipTag==SkipTag::YES},val);
}

//...... блок операторов вывода ..............
/// \todo передать обязанность выставления точности на пользователя (!)

ostream& operator<<(ostream& s, const Object& o)  {
    stringstream ss;
                      ss.precision(s.precision());
                      ss << o;
    return s<<ss.str();
}
ostream& operator<<(ostream& s, const Array& a)   {
    stringstream ss;
                      ss.precision(s.precision());
                      ss << a;
    return s<<ss.str();
}
ostream& operator<<(ostream& s, const Record& r)  {
    stringstream ss;
                      ss.precision(s.precision());
                      ss  << r;
    return s<<ss.str();
}
stringstream& operator<<(stringstream& ss, const Object& o)
{
    ss << boolalpha;
    writeValue(ss,0,"",o,SkipTag::YES);
    return ss;
}
stringstream& operator<<(stringstream& ss, const Array & a)
{
    ss << boolalpha;
    writeValue(ss,0,"",a,SkipTag::YES);
    return ss;
}
stringstream& operator<<(stringstream& ss, const Record& r)
{
    ss << boolalpha;
    writeValue(ss,0,r.first,r.second,SkipTag::NO);
    return ss;
}
//...........................................
//-------------------------------------------

/**
 * \return строку вида "tag : value"
 */
string to_string(const Record &r)
{
    static const int EXT_PRECISION = 12;    //увеличим точность вывода double до EXT_PRECISION знаков. Установите другое значение по необходимости.
    stringstream ss;
    ss<<setprecision(EXT_PRECISION);
    ss<<boolalpha;
    writeValue(ss,0,r.first,r.second,SkipTag::NO);
    return ss.str();
}

#ifdef JJSON17_PARSE
//=======================================================================================================
//static void skip(istream &s,char terminator) { s.ignore(numeric_limits<streamsize>::max(),terminator); }
template<typename FwdIterator>
static auto skip(istream &s,FwdIterator termsBegin,FwdIterator termsEnd)
{
    return find_if(istream_iterator<char>(s),istream_iterator<char>(),
                     [termsBegin,termsEnd] (char c)
                        { return find(termsBegin,termsEnd,c)!=termsEnd;} );
};
// exclude terminators
static string readline(istream &s,char terminator1,char terminator2)
{
    string res;
    char c{'\0'};    // clang попросил тут иницализировать тк почему то считает что если ниже s>>c не получится, то сравнение будет с мусором, но s>>c должен получится, ведь я сначала проверил s
    while(s>>c && c!=terminator1 && c!=terminator2)
        res.push_back(c);
    s.unget();
    return res; //NRVO
}
static string readline_escaped(istream &s,char terminator)
{
    string res;
    char c;
    while(s) {
        c = s.get();
        if(c == terminator) break;
        if(c=='\\')  {
            c = s.get();
            switch (c) {    //здесь можно соптимизировать по таблице
                case 'b'  : res.push_back('\b');break;
                case 'f'  : res.push_back('\f');break;
                case 'n'  : res.push_back('\n');break;
                case 'r'  : res.push_back('\r');break;
                case 't'  : res.push_back('\t');break;
                default   : res.push_back(c); break;
            }
        }
        else res.push_back(c);
    }
    return res; //NRVO
}

static Value  parseNumber (istream& s);
static Array  parseArray  (istream& s);
static Object parseObject(istream& s);
static Value  parseLiteral(string&& str);
static Value  parseValue(char c, istream& s)
{
    if(c=='-'||isdigit(c))  return parseNumber(s);
    else if(c=='"')         return readline_escaped(s,'"');
    else if(c=='{')         return parseObject(s);
    else if(c=='[')         return parseArray(s);
    else                    return parseLiteral(readline(s,',','}'));
}
static Object parseObject(istream& s)
{
    using namespace std;
    static const array<char,2> START{'}','"'};

    auto last = skip(s,START.begin(),START.end());
    if(!s) throw runtime_error(string("invalid json syntax ")+__FUNCTION__+" L"+std::to_string(__LINE__));
    if(*last == '}') return {};

    auto pObj = make_unique<Object>();
    char c;
    Value v;
    do {
        s >> skipws;
        auto tag = readline_escaped(s,'"');
        s>>c;// считаем ':' //if(c!=':') throw runtime_error("invalid json syntax");
        s>>c;
        v = parseValue(c,s);//тут какая то ошибка !!!!!!!
        pObj->insert({tag,v});
        c = *skip(s,START.begin(),START.end());
    } while(s && c!='}');

    return move(*pObj);
}

static Array parseArray  (istream& s)
{
    using namespace std;
    auto pArr = make_unique<Array>();
    char c;
    Value v;
    s>>skipws;
    do {
        s>>c;
        if(c==']') break;
        v= parseValue(c,s);
        pArr->push_back(v);
        s>>c;   // считаем ','
    } while(s && c!=']');

    return move(*pArr);
}

static Value parseNumber (istream& s)
{
    s.unget();
    double n;
    s >> n;
    if(floor(n) < n)
         return n ;
    else return static_cast<int64_t>(n);
}
static Value parseLiteral(string &&str)
{
    if(str.size() > 2)
    {
        if(str[0]=='u'&&str[1]=='l'&&str[2]=='l')
            return nullptr;
        if(str[0]=='r'&&str[1]=='u'&&str[2]=='e')
            return true;
        if(str.size() > 3 && str[0]=='a'&&str[1]=='l'&&str[2]=='s'&&str[3]=='e')
            return false;
    }
    throw runtime_error(string("invalid json syntax ")+__FUNCTION__+" L"+std::to_string(__LINE__));
}

Value parse(istream& s)
{
    char c;
    s>>c;
    switch (c) {
        case '{': return parseObject(s); // |>  in RVO we trust
        case '[': return parseArray (s); // |
        default : throw runtime_error(string("invalid json syntax ")+__FUNCTION__+" L"+std::to_string(__LINE__));
    }
}
//=================================================================================
#endif

} // end of ns jjson17

