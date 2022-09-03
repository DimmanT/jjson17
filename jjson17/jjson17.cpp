#include "jjson17.h"

#include <sstream>
#include <iomanip>

namespace jjson17 {


//---------- блок Value ---------------
inline constexpr Type type_of(const Value_t& value)
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

void writeValue(std::ostream& s,int depth, const std::string& tag,const Value_t& val, SkipTag skipTag);

/**
 * @brief escape
 * @param str
 * @return строку в UTF-8 с экранированными символами согласно рекомендации \link json.org
 */
std::string escape(std::string&& str)
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
                // '"' будут обработаны отдельно функцией quoted()
                default   : break;
            }
        }
        i+=mv;  // сдвигаемся на количество байт соотв. размеру UTF-8 символа
    }

    return std::move(str);
}
inline auto prepareStr(std::string str)
{
    return '"'+escape(std::move(str))+'"';
}

/**
 * @brief writeValue рекурсивная функция, заполняющая поток \c s значением \c val и вызываемая для каждого подзначения, хранимого в \c val.
 * @param s         поток, в который будут записываться значения.
 * @param depth     отражает глубину вложенности, определяет число отступов ('tab') вставляемых перед значением.
 * @param tag       наименование значения. Можно оставить пустым и выставить параметр \c skipTag в \c SkipTag::YES, тогда наименование будет опущено и символ ':' не будет напечатан.
 * @param v         само значение
 * @param skipTag   если выставлен в \c SkipTag::YES , то в поток будет записано только значение без, наименования и символа ':'
 */
void writeValue(std::ostream& s,int depth, const std::string& tag,const Value_t& val, SkipTag skipTag)
{
  #define INSERT_TABS  for(int t = 0; t < depth; ++t)  s<<'\t'

    using namespace std;
    if(skipTag == SkipTag::NO)
        s << prepareStr(tag)<<':'<<'\t';
    switch (type_of(val)) {
    case Type::NUL      :   s <<"null";break;   //non-quoted
    case Type::BOOL     :   s <<std::boolalpha<<get<bool>(val)<<std::noboolalpha;break;
    case Type::STRING   :   s <<prepareStr(get<string>(val));break;
    case Type::INTEGER  :   s <<get<int64_t>(val);break;
    case Type::REAL     :   s <<get<double>(val);break;
    case Type::ARRAY    :   {
                            INSERT_TABS;
                            s << '[' <<endl;
                            const auto& arr = get<Array>(val);
                            if(!arr.empty())
                            {
                                depth++;
                                for(size_t i = 0; i < arr.size()-1;++i)
                                {
                                    INSERT_TABS;
                                    writeValue(s,depth,"",arr[i],SkipTag::YES);
                                    s<<','<<endl;
                                }
                                INSERT_TABS;                                    // |
                                writeValue(s,depth,"",arr.back(),SkipTag::YES); // |>  то же, но без ','
                                s<<endl;                                        // |
                                depth--;
                            }
                            INSERT_TABS;
                            s << ']';
                            break;
                            }
    case Type::OBJECT   :   {
                            INSERT_TABS;
                            s<<'{'<<endl;
                            const auto& obj = get<Object>(val);
                            if(!obj.empty())
                            {
                                depth++;
                                auto last = obj.end();last--;
                                for(const auto& [k,v] : obj)
                                {
                                    INSERT_TABS;
                                    writeValue(s,depth,k,v,SkipTag::NO);
                                    if(k!=last->first) s<<',';
                                    s<<endl;
                                }
                            }
                            depth--;
                            INSERT_TABS;
                            s<<'}';
                            break;
                            }
    }
#undef INSERT_TABS
}

//...... блок операторов вывода ..............
static const int EXT_PRECISION = 12;    //увеличим точность вывода double до EXT_PRECISION знаков. Установите другое значение по необходимости.

std::ostream& operator<<(std::ostream& s, const Object& o)
{
    s <<std::setprecision(EXT_PRECISION);
    writeValue(s,0,"",o,SkipTag::YES);
    return s;
}
std::ostream& operator<<(std::ostream& s, const Array & a)
{
    s <<std::setprecision(EXT_PRECISION);
    writeValue(s,0,"",a,SkipTag::YES);
    return s;
}
std::ostream& operator<<(std::ostream& s, const Record& r)
{
    s <<std::setprecision(EXT_PRECISION);
    writeValue(s,0,r.first,r.second,SkipTag::NO);
    return s;
}
// + еще один в пространстве std
//...........................................
//-------------------------------------------

/**
 * \return строку вида "tag : value"
 */
std::string to_string(const Record &r)
{
    std::stringstream ss;
    ss<<std::setprecision(EXT_PRECISION);
    writeValue(ss,0,r.first,r.second,SkipTag::NO);
    return ss.str();
}

} // end of ns jjson17

