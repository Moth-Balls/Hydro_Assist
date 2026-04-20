#pragma once

#include <ArduinoJson.h>
#include <String>
#include <LittleFS.h>
#include "plant_profile.hpp"


TargetPlant find_plant_data(std::string plant_name) {

    StaticJsonDocument<2048> profile;

    JsonArray plantArray = profile["plant_nutrient_data"];


    TargetPlant pulled_plant;
    pulled_plant.name = plant_name;

    //TODO: Add logic to take json data and put into struct
    pulled_plant.ec_high;
    pulled_plant.ec_low;
    pulled_plant.ec_avg;

    pulled_plant.ph_high;
    pulled_plant.ph_low;
    pulled_plant.ph_avg;
    
    













}

