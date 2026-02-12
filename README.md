# Game-server
Многопользовательский игровой сервер, написанный на C++, который позволяет игрокам управлять собаками на виртуальных картах, собирать предметы (лут), соревноваться в очках и взаимодействовать через HTTP API.

### 🛠 Project Tech Stack

| Component | Technologies & Tools |
| :--- | :--- |
| **Language & Standard** | ![C++20](https://img.shields.io/badge/C++20-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white) |
| **Networking** | ![Boost.Asio](https://img.shields.io/badge/Boost.Asio-Async-00599C?style=for-the-badge&logo=boost&logoColor=white) ![Boost.Beast](https://img.shields.io/badge/Boost.Beast-HTTP/1.1-00599C?style=for-the-badge&logo=boost&logoColor=white) |
| **Data & Persistence** | ![PostgreSQL](https://img.shields.io/badge/PostgreSQL-4169E1?style=for-the-badge&logo=postgresql&logoColor=white) ![Boost.JSON](https://img.shields.io/badge/Boost.JSON-Parser-00599C?style=for-the-badge&logo=boost&logoColor=white) ![Boost.Serialization](https://img.shields.io/badge/Boost.Serialization-Binary-00599C?style=for-the-badge&logo=boost&logoColor=white) |
| **Architecture** | ![Clean Architecture](https://img.shields.io/badge/Clean_Architecture-Layers-blueviolet?style=for-the-badge) ![Dependency Injection](https://img.shields.io/badge/Pattern-DI-blueviolet?style=for-the-badge) ![Repository](https://img.shields.io/badge/Pattern-Repository-blueviolet?style=for-the-badge) |
| **DevOps & Build** | ![CMake](https://img.shields.io/badge/CMake-System-064F8C?style=for-the-badge&logo=cmake&logoColor=white) ![Conan](https://img.shields.io/badge/Conan-Package_Manager-6699CB?style=for-the-badge&logo=conan&logoColor=white) |
| **Diagnostics & QA** | ![Boost.Log](https://img.shields.io/badge/Boost.Log-JSON_Structured-00599C?style=for-the-badge&logo=boost&logoColor=white) ![Catch2](https://img.shields.io/badge/Catch2-Unit_Testing-000000?style=for-the-badge) |
| **Features** | ![Multithreading](https://img.shields.io/badge/Concurrency-Strands_/_Multithreading-orange?style=for-the-badge) ![Collision Detection](https://img.shields.io/badge/Logic-Collision_Detection-green?style=for-the-badge) |

## Требования для сборки

### Обязательное ПО

- **Компилятор C++** с поддержкой **C++20:**
  - GCC 10+, Clang 12+, MSVC 2019 16.11+ (Windows)
- **CMake** версии **3.11** или выше
- **Conan** — менеджер пакетов (версия 1.x или 2.x)

Установка:
```
pip install conan
```
- **PostgreSQL** — библиотеки для разработки (будут автоматически загружены через **Conan**, но для запуска сервера требуется работающий экземпляр БД):
  - Ubuntu/Debian: `libpq-dev`, `libpqxx-dev`
  - Windows: через **vcpkg** или установщик **EDB**
  - macOS:
```
brew install postgresql libpqxx
 ```

### Дополнительно для запуска тестов
- **Catch2** — будет загружен через Conan

## Получение зависимостей
Управление зависимостями осуществляется через **Conan**.

Все необходимые библиотеки (Boost, libpqxx, Catch2) описаны в файле `conanfile.txt`.

### Установка зависимостей
Из корня проекта выполните:
```
mkdir build
cd build
conan install .. --build=missing -s build_type=Release -s compiler.libcxx=libstdc++11 # или Debug
```

## Сборка проекта
После установки зависимостей выполните конфигурацию и сборку через **CMake**.

### Linux / macOS
```
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

### Windows (MSVC)
```
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
cmake --build . --config Release
```
После успешной сборки в каталоге `build` появится исполняемый файл:
- **Linux / macOS**: `game_server`
- **Windows**: `game_server.exe`

Также будет собран набор модульных тестов: `game_server_tests`.

## Настройка базы данных
Сервер сохраняет завершённые игры (рекорды) в **PostgreSQL**.

Перед запуском необходимо:
1. Установить и запустить PostgreSQL (локально или удалённо).
2. Создать базу данных (например, `game_db`).
3. Задать переменную окружения `GAME_DB_URL` в формате:
```
postgresql://username:password@host:port/database
```
Пример:

`export GAME_DB_URL=postgresql://postgres:secret@localhost:5432/game_db`

При первом обращении сервер автоматически создаст таблицу retired_players и необходимые индексы.

## Запуск сервера
Сервер принимает следующие параметры командной строки:

### 🚀 Конфигурация запуска сервера

| Параметр | Описание | Обязательный |
| :--- | :--- | :--- |
| ` -c `, `--config-file` | Путь к JSON-конфигу (карты, лут и правила игры) | ![Да](https://img.shields.io/badge/ОБЯЗАТЕЛЬНО-red?style=for-the-badge) |
| ` -w `, `--www-root` | Путь к директории статики (HTML, CSS, JS) | ![Да](https://img.shields.io/badge/ОБЯЗАТЕЛЬНО-red?style=for-the-badge) |
| ` -f `, `--state-file` | Файл для сохранения и восстановления состояния игры | ![Нет](https://img.shields.io/badge/ОПЦИОНАЛЬНО-grey?style=for-the-badge) |
| ` -t `, `--tick-period` | Период авто-такта в **мс** (по умолчанию — через API) | ![Нет](https://img.shields.io/badge/ОПЦИОНАЛЬНО-grey?style=for-the-badge) |
| ` -s `, `--save-period` | Интервал сохранения в **мс** (нужен `--state-file`) | ![Нет](https://img.shields.io/badge/ОПЦИОНАЛЬНО-grey?style=for-the-badge) |
| `--randomize-spawn` | Включить случайные точки появления игроков | ![Нет](https://img.shields.io/badge/ОПЦИОНАЛЬНО-grey?style=for-the-badge) |
| ` -h `, `--help` | Показать справку и выйти | ![Нет](https://img.shields.io/badge/ОПЦИОНАЛЬНО-grey?style=for-the-badge) |

### Пример запуска
```
./game_server \
  --config-file ./data/config.json \
  --www-root ./static \
  --state-file ./saves/state.txt \
  --tick-period 100 \
  --save-state-period 5000 \
  --randomize-spawn-points
```
После запуска сервер будет принимать HTTP-запросы **на порту 8080** (фиксировано).
```
http://localhost:8080
```

## Тестирование
Для запуска модульных тестов используйте **CTest** или запустите `game_server_tests` напрямую:
```
cd build
ctest -C Release    # Windows
ctest               # Linux / macOS
```
Или вручную:
```
./game_server_tests
```
Тесты покрывают:
- Генерацию лута (`loot_generator_tests.cpp`)
- Детектор коллизий (`collision-detector-tests.cpp`)
- Сериализацию состояния (`state-serialization-tests.cpp`)

Все тесты должны завершаться успешно.

## Конфигурация игры (JSON)
Файл конфигурации (`--config-file`) содержит:
- Список карт (`maps`) с дорогами, зданиями, офисами, типами лута, скоростью собак, вместимостью рюкзака
- Глобальные настройки: `defaultDogSpeed`, `defaultBagCapacity`, `dogRetirementTime`, `lootGeneratorConfig`
