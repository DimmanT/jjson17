#ifndef JJSON_17_HEADER
#define JJSON_17_HEADER
//UTF-8

#include <map>
#include <vector>
#include <variant>
#include <string>
#include <sstream>

#define JJSON17_PARSE

namespace jjson17
{
    enum class Type : unsigned char {NUL,STRING,INTEGER,REAL,BOOL,ARRAY,OBJECT};    ///< перечисление возможных типов хранимых в объекте \ref Value
    struct Value;
    using  Record = std::pair<std::string,Value>;       ///< псевдоним для записи в объектах

    using Array   = std::vector<Value>;                 ///< псевдоним для массивов
    using Object  = std::map<std::string,Value>;        ///< псевдоним для объектов

    using Value_t = std::variant<std::nullptr_t,std::string,int64_t,double,bool,Array,Object>;  ///< псевдоним для значения любого из указанных типов

    /**
     * \brief Структура с дополнительными функциями
     */
    struct Value : public Value_t
    {
        using Value_t::Value_t;
        Value() : Value_t(nullptr) {}
        Value(const char* cstr) : Value_t(std::string(cstr)) {}
        template<typename T,typename = std::enable_if_t<!std::is_same_v<T,bool> && std::is_integral_v<T>> >
        Value(T v) : Value_t(int64_t(v)) {}
        Value(int64_t v) : Value_t(v) {}

        void push_back(Value v);    ///< \brief дописывает значение \c v в конец массива. \c *this при этом обязан быть массивом (\ref Array).
        void insert(Record r);      ///< \brief добавляет запись \c r в объект. \c *this при этом обязан быть объектом (\ref Object).
        bool isNull();              ///< \return \c true , если *this имеет тип NUL, \c false иначе
    };

    // --- функции вывода в поток ---
    std::ostream& operator<<(std::ostream& s, const Object& o);
    std::ostream& operator<<(std::ostream& s, const Array & a);
    std::ostream& operator<<(std::ostream& s, const Record& r);

    //... спец. функции для записи в строковый поток ...
    std::stringstream& operator<<(std::stringstream& ss, const Object& o);
    std::stringstream& operator<<(std::stringstream& ss, const Array & a);
    std::stringstream& operator<<(std::stringstream& ss, const Record& r);
    // ------------------------------

    #ifdef JJSON17_PARSE
    Value parse(std::istream& s);   ///< \brief парсит поток формируя дерево JSON и возвращает его. \return Может быть либо объектом \ref jjson17::Object либо массивом \ref jjson17::Array
    #endif

    std::string to_string(const Record& r); ///< \brief формирует строку вида ""tag" : value". Строка в кодировке UTF-8.

}//end of NS jjson17

#endif //JJSON_17_HEADER
