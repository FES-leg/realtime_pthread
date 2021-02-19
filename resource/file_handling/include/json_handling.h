/** @file
 * @brief Contains declarations required to read and load parameters from json configuration files.
*/

#ifndef JSON_HANDLING_H
#define JSON_HANDLING_H


/*****************************************************************************************
 * INCLUDES
 ****************************************************************************************/
#include "read_file_exception_handling.h"

/*****************************************************************************************
 * CLASSES
 ****************************************************************************************/

class json_object
{
public:
    json_object(){};

    void populate(nlohmann::json);
    config_file_exception check_parameters();

    //Casting specific types
    template <typename T>
    T get_scalar(const std::string);
    template <typename T>
    T get_matrix(const std::string,unsigned rows, unsigned cols);
    template <typename T>
    T get_vector(const std::string);

    void set_exception_labels(const std::string, const std::string);

private:
    nlohmann::json parameter_list;
    config_file_exception_handler exception_handler;

    double get_scalar_double(const std::string);
    Eigen::Vector3d get_vector3d(const std::string);
    Eigen::MatrixXd get_matrix_X_rows_Y_cols(const std::string, const unsigned, const unsigned);
    config_file_exception error_handling(const std::string,const std::string, const unsigned elements = 0);
};

class json_data_file
{

public:
    json_data_file(){};

    config_file_exception parse(std::string, int n_objects = -1);
    json_object get_object(const std::string);
    unsigned get_number_objects();

private:
    nlohmann::json root;
    config_file_exception_handler exception_handler;

    config_file_exception read_file(std::ifstream&, const int);
    config_file_exception object_error_handling(const std::string);
    config_file_exception file_error_handling(const int);

};


#endif // JSON_HANDLING_H
