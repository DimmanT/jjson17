#include "jjson17.h"

#include <sstream>
#include <iomanip>

namespace jjson17 {


//---------- блок Value ---------------
static constexpr Type type_of(const Value_t& value)
{
    return static_cast<Type>(value.index());    //следует следить за порядком типов в объявлении варианта
}

/**
 * \details Если значение \c *this не является массивом, то будет выброшено исключение типа \c std::logic_error .
 */
void Value::push_back(Value v)
{
    Type t = type_of(*this);
    if(t == Type::ARRAY)
         std::get<Array>(*this).push_back(v);
    else throw std::logic_error("This value IS NOT an ARRAY. Rebuilding into array is not supported yet.");
}

/**
 * \details Если значение \c *this не является объектом, то будет выброшено исключение типа \c std::logic_error .
 */
void Value::insert(Record r)
{
    Type t = type_of(*this);
    if(t == Type::OBJECT)
         std::get<Object>(*this).insert(r);
    else throw std::logic_error("This value IS NOT an OBJECT. Rebuilding into object is not supported yet.");
}

bool Value::isNull()
{
    return type_of(*this) == Type::NUL;
}
//--------------------------------------

//----------- блок Вывода --------------
enum class SkipTag : bool {NO,YES};

static void writeValue(std::ostream& s,uint16_t depth, const std::string& tag,const Value_t& val, SkipTag skipTag);

/**
 * @brief escape
 * @param str
 * @return строку в UTF-8 с экранированными символами согласно рекомендации \link json.org
 */
static std::string escape(std::string&& str)
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

    return std::move(str);
}
static auto prepareStr(std::string str)
{
    return '"'+escape(std::move(str))+'"';
}

struct Visitor {
    Visitor(std::ostream& stream, uint16_t depth, bool skipTag) : s(stream),depth(depth),skipTag(skipTag) {}
    void operator()(nullptr_t             ) {s << "null";}
    void operator()(bool               val) {s << val;}
    void operator()(const std::string& str) {s << prepareStr(str);}
    void operator()(int64_t            num) {s << num;}
    void operator()(double             num) {s << num;}
    void operator()(const Array&       arr)
    {
        using namespace std;
        if(!skipTag) {
            s << std::endl;
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
        using namespace std;
        if(!skipTag)  {
            s << std::endl;
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
    std::ostream& s;
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
static void writeValue(std::ostream& s,uint16_t depth, const std::string& tag,const Value_t& val, SkipTag skipTag)
{
    if(skipTag == SkipTag::NO)
        s << prepareStr(tag)<<':'<<'\t';
    std::visit(Visitor{s, depth, skipTag==SkipTag::YES},val);
}

//...... блок операторов вывода ..............
/// \todo передать обязанность выставления точности на пользователя (!)

std::ostream& operator<<(std::ostream& s, const Object& o)  {
    std::stringstream ss;
                      ss.precision(s.precision());
                      ss << o;
    return s<<ss.str();
}
std::ostream& operator<<(std::ostream& s, const Array& a)   {
    std::stringstream ss;
                      ss.precision(s.precision());
                      ss << a;
    return s<<ss.str();
}
std::ostream& operator<<(std::ostream& s, const Record& r)  {
    std::stringstream ss;
                      ss.precision(s.precision());
                      ss  << r;
    return s<<ss.str();
}
std::stringstream& operator<<(std::stringstream& ss, const Object& o)
{
    ss << std::boolalpha;
    writeValue(ss,0,"",o,SkipTag::YES);
    return ss;
}
std::stringstream& operator<<(std::stringstream& ss, const Array & a)
{
    ss << std::boolalpha;
    writeValue(ss,0,"",a,SkipTag::YES);
    return ss;
}
std::stringstream& operator<<(std::stringstream& ss, const Record& r)
{
    ss << std::boolalpha;
    writeValue(ss,0,r.first,r.second,SkipTag::NO);
    return ss;
}
//...........................................
//-------------------------------------------

/**
 * \return строку вида "tag : value"
 */
std::string to_string(const Record &r)
{
    static const int EXT_PRECISION = 12;    //увеличим точность вывода double до EXT_PRECISION знаков. Установите другое значение по необходимости.
    std::stringstream ss;
    ss<<std::setprecision(EXT_PRECISION);
    ss<<std::boolalpha;
    writeValue(ss,0,r.first,r.second,SkipTag::NO);
    return ss.str();
}

} // end of ns jjson17

