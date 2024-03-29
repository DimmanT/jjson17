#ifndef JJSON_17_HEADER
#define JJSON_17_HEADER
//UTF-8

#include <map>
#include <vector>
#include <variant>
#include <string>
#include <sstream>

#ifdef JJSON17_PARSE
#include <cmath>    //для округления в operator T ()
#endif

namespace jjson17
{
    enum class Type : unsigned char {NUL,STRING,INTEGER,REAL,BOOL,ARRAY,OBJECT};    ///< перечисление возможных типов хранимых в объекте \ref Value . Можно получить методом std::variant::index()
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
        Value(Value& v)                : Value_t(static_cast<const Value_t&>(v)){}  //это нужно тк выигрывается перегрузка именнно неконстантной ссылки
        Value(const Value& v)          : Value_t(static_cast<const Value_t&>(v)) {}
        Value(Value&& v)                 = default;
        Value& operator=(const Value& v) = default;
        Value& operator=(Value&& v)      = default;

        Value() : Value_t(nullptr) {}
        Value(unsigned long long v) : Value_t(static_cast<int64_t>(v)) {}

        void push_back(Value v);    ///< \brief дописывает значение \c v в конец массива. \c *this при этом обязан быть массивом (\ref Array).
        void insert(Record r);      ///< \brief добавляет запись \c r в объект. \c *this при этом обязан быть объектом (\ref Object).
        bool isNull();              ///< \return \c true , если *this имеет тип NUL, \c false иначе

        #ifdef JJSON17_PARSE
        /// \brief Обеспечивает неявное преобразование к целому числу (знак. и беззнак.). Если не хранит число, то бросает \c std::bad_variant_access .
        /// \return число хранимое в структуре. Вещественные числа матем-ки округляются.
        template<typename T,typename = std::enable_if_t<!std::is_same_v<T,bool> && std::is_integral_v<T>> >
        operator T () const {
            switch (static_cast<Type>(this->index())) {
              case Type::REAL   : return std::round(std::get<double>(*this));
              case Type::INTEGER: return std::get<int64_t>(*this);
              default: throw std::bad_variant_access();
        }}
        operator double     () const;
        operator std::string() const;
        operator bool       () const;
        operator Array      () const;
        operator Object     () const;
        #endif
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
