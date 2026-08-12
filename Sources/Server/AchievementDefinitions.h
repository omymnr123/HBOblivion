#pragma once
#include <string>

struct AchievementData {
    int id;
    std::string title;
    std::string description;
    int reward_type; // 1 = Título, 2 = Ítem/Skin
};

// Ejemplo de lista estática con tus primeros logros
inline const AchievementData kAchievements[] = {
    { 1, "Novato Valiente", "Alcanza el nivel 10 por primera vez.", 1 },
    { 2, "Veterano de Guerra", "Derrota a tu primer jefe en el mundo.", 1 },
    { 3, "Coleccionista", "Consigue un objeto legendario.", 2 }
};