# jjson17
## Назначение
 Проект реализует простейшую сериализацию (запись в файл) данных в json на языке C++ (стандарт 17). Данное решение спроектировано для применения в простых задачах без больших уровней вложенности и экстремальных объемов переводимых в JSON-формат данных.
 
 В настоящий момент парсинг json-данных не поддерживается.

## Использование
 1. Включите файлы **jjson17.h** и **jjson17.cpp** в ваш проект.
 2. Если вам нужно сохранить в файл объект класса или структуры, то воспользуйтесь псевдонимом jjson17::Object.
 
 	Пусть сохраняемая структура выглядит 	следующим образом:
 
		struct Struct {
		    std::string somestr;
		    short       someval;
		    Struct*     other{nullptr};
		};
	Тогда преобразовать ее в json-объект можно следующим образом:
	
		jjson17::Object asJsonObject(const Struct& s)
		{
    	    namespace json = jjson17;
		    json::Object obj;
                 	 obj.insert({"somestr",s.somestr});
                 	 obj.insert({"someval",s.someval});
		    if(s.other)
         	 obj.insert({"other",asJsonObject(*s.other)});
    	    else obj.insert({"other",s.other});
    	    return obj;
		};
	Также вы можете создать объект списком инициализации:
	
	    json::Object obj{
                {"somestr",s.somestr},
                {"someval",s.someval},
                };
3. Полученный объект можно сохранить в файл воспользовавшись перегруженным  **operator<<()** :

	    std::ofstream ofs;
                      ofs.open("outfile.json");
                      ofs << obj;
                      ofs.close();
 
 Примеры использования можно посмотреть в [проекте автотестирования](https://github.com/DimmanT/jjson17-tests).

## Валидация
Проверка корректности сериализации данных проведена парсингом сериализованных данных средствами QJson фреймворка Qt. Проект автотестирования доступен по [ссылке](https://github.com/DimmanT/jjson17-tests) и как git-подмодуль к настоящему проекту.

 Сборка проверена на компиляторах от **MSVS2019** и **MinGW 8.1.0** , как в 32, так и в 64 битной версии (CPU x86-64). Сборка и валидация под Linux не проводилась. 
 
## Требования
 Требуется компилятор поддерживающий стандарт C++17.
 Для сборки [проекта автотестирования](https://github.com/DimmanT/jjson17-tests) также понадобится Qt 5.13.
 
 Ожидается, что все строки, которыми оперируют классы проекта, хранят символы в формате **UTF-8**.

## Дополнительная информация
* Ключевым элементом 17-го стандарта, используемым в проекте является **std::variant**.
* Поскольку при записи данных в поток ввода-вывода используется рекурсия, данное решение может не подойти для задач сериализации структур с большим уровнем вложенности.
* Перед записью в файл применяется оптимизация, перенаправляющая вывод сначала в строку. Это приводит к формированию строки большой длины, которая хранится в оперативной памяти. Не рекомендую данное решение для формирования JSON-файлов размерами в сотни Мб и более.