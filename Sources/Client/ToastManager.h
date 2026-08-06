#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

// Esta pequeña estructura es "la nota" que lleva el camarero.
// Guarda el texto y cuánto tiempo lleva en pantalla.
struct ActiveToast {
    std::string title;
    std::string desc;
    float timer; // Un cronómetro que empieza en 0
};

class ToastManager {
public:
    // Aquí guardamos todas las notas que nos lleguen
    std::vector<ActiveToast> m_active_toasts;
    
    // Necesitamos una fuente de texto para poder escribir
    sf::Font m_font;

    // --- CONSTRUCTOR ---
    // Esto se ejecuta una sola vez cuando abres el juego.
    ToastManager() {
        // Le decimos a SFML qué tipo de letra usar. 
        // ¡IMPORTANTE! Asegúrate de tener un archivo .ttf en la carpeta de tu juego.
        if (!m_font.loadFromFile("arial.ttf")) { 
            // Si falla, el juego avisará en la consola, pero no crasheará.
        }
    }

    // --- FUNCIÓN PARA AÑADIR LOGROS ---
    // Cuando el Servidor nos diga "¡Logro conseguido!", llamaremos a esta función.
    void AddToast(const std::string& title, const std::string& desc) {
        // Metemos una nueva nota en la bandeja del camarero.
        // Empieza con el temporizador a 0.0f
        m_active_toasts.push_back({ title, desc, 0.0f });
    }

    // --- FUNCIÓN PARA DIBUJAR (RENDER) ---
    // Esta función se va a llamar TODO EL TIEMPO, 60 veces por segundo, 
    // mientras juegas, igual que se dibuja el mapa o tu personaje.
    void Render(sf::RenderWindow& window, float deltaTime) {
        
        // Vamos a revisar todas las notas que tenemos guardadas
        for (int i = 0; i < m_active_toasts.size(); i++) {
            
            // Cogemos la nota actual
            auto& toast = m_active_toasts[i];
            
            // Le sumamos el tiempo que ha pasado desde el último frame
            toast.timer += deltaTime;

            // Si la nota lleva más de 4 segundos en pantalla...
            if (toast.timer > 4.0f) {
                // ¡La borramos de la bandeja!
                m_active_toasts.erase(m_active_toasts.begin() + i);
                i--; // Ajustamos el índice porque acabamos de borrar una
                continue; // Pasamos a la siguiente nota
            }

            // --- DIBUJAMOS EL FONDO OSCURO ---
            // Creamos un rectángulo de 300x60 píxeles
            sf::RectangleShape bg(sf::Vector2f(300, 60));
            
            // Lo centramos arriba en la pantalla. 
            // Si hay varias notas a la vez, el "i * 70" hace que se pongan una debajo de otra.
            bg.setPosition(window.getSize().x / 2.0f - 150.0f, 20.0f + (i * 70.0f)); 
            
            // Le damos color: Gris oscuro (20, 20, 20) y un poco transparente (200 de 255)
            bg.setFillColor(sf::Color(20, 20, 20, 200)); 
            
            // Le ponemos un borde dorado de 2 píxeles de grosor
            bg.setOutlineThickness(2.0f);
            bg.setOutlineColor(sf::Color(255, 215, 0)); 

            // --- DIBUJAMOS EL TÍTULO (DORADO) ---
            sf::Text titleText(toast.title, m_font, 18);
            titleText.setPosition(bg.getPosition().x + 10, bg.getPosition().y + 5);
            titleText.setFillColor(sf::Color(255, 215, 0)); // Color Dorado

            // --- DIBUJAMOS LA DESCRIPCIÓN (BLANCO) ---
            sf::Text descText(toast.desc, m_font, 14);
            descText.setPosition(bg.getPosition().x + 10, bg.getPosition().y + 30);
            descText.setFillColor(sf::Color::White); // Color Blanco

            // --- PINTAMOS TODO EN LA VENTANA ---
            window.draw(bg);
            window.draw(titleText);
            window.draw(descText);
        }
    }
};