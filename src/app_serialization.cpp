#include "app_serialization.h"

namespace serialization {

using namespace std::literals;

void AppSerialization(const std::filesystem::path& file_to_serialize_, app::Application& app) {
    auto temp_file = file_to_serialize_.parent_path();
    temp_file  += ("/temp_file");

    std::ofstream out(temp_file , std::ios_base::binary);
    boost::archive::text_oarchive output{out};
    ApplicationRepr app_repr(app);
    output << app_repr;

    std::filesystem::rename(temp_file , file_to_serialize_);
}

void AppDeserialization(const std::filesystem::path& file_to_serialize_, app::Application& app) {
    try {
        if(!std::filesystem::exists(file_to_serialize_)){
            return;
        }

        std::ifstream in(file_to_serialize_, std::ios_base::binary);
        if (!in.is_open()) {
            throw std::ios_base::failure("Save file is not open");
        }

        boost::archive::text_iarchive input{in};          
        ApplicationRepr app_repr;
        input >> app_repr;
        app_repr.Restore(app);
    }
    catch(const std::exception& e) {
        throw std::ios_base::failure(e.what());
    }
}
    
}