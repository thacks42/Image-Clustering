#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <limits>
#include <algorithm>
#include <utility>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include "image_clusterer.hpp"


int main(int argc, char** argv){
    size_t width = 1600;
    size_t height = 1024;
	sf::RenderWindow window(sf::VideoMode(width, height), "SFML window");
    window.setFramerateLimit(60);
	
    std::string output_file_name = "result.png";
    
    if(argc < 2){
        std::cout<<"usage: ./program name_of_image [optional name of ouput image file]\n";
        return -1;
    }
    if(argc == 3){
        output_file_name = argv[2];
    }
    
    sf::Image pic;
    if(!pic.loadFromFile(argv[1])) return -1;
    
    sf::Font font;
    font.loadFromFile("DejaVuSansMono.ttf");
    
    sf::Text help_text;
    help_text.setFont(font);
    help_text.setString("Up -> increment bins\n"\
                        "Down -> decrement bins\n"\
                        "Right -> increment gamma\n"\
                        "Left -> decrement gamma\n"\
                        "Space -> save image\n"\
                        "M -> apply colormap optimized for LC\n"\
                        "A -> toggle ignore-alpha for clustering\n"\
                        "H -> show/hide this help text");
    
    help_text.setCharacterSize(24);
    help_text.setFillColor(sf::Color::White);
    help_text.setOutlineColor(sf::Color::Black);
    help_text.setOutlineThickness(2.0f);
    
    bool show_help_text = true;
    
    image_clusterer clusterer(pic, width, height);
    
    //sf::View view = window.getDefaultView();
    
	while(window.isOpen()){
        sf::Event event;
        while (window.pollEvent(event)){
            if(event.type == sf::Event::Closed){
                window.close();
			}
            if(event.type == sf::Event::Resized){
                clusterer.resize_window(event.size.width, event.size.height);
                 sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
                window.setView(sf::View(visibleArea));
            }
			if(event.type == sf::Event::KeyPressed){
				if (event.key.code == sf::Keyboard::Up){
                    clusterer.increment_bins();
				}
                if(event.key.code == sf::Keyboard::Down){
                    clusterer.decrement_bins();
				}
                if(event.key.code == sf::Keyboard::Left){
                    clusterer.decrement_gamma();
                }
                if(event.key.code == sf::Keyboard::Right){
                    clusterer.increment_gamma();
                }
                if(event.key.code == sf::Keyboard::Space){
                    clusterer.modified_image.tex.copyToImage().saveToFile(output_file_name);
                }
                
                if(event.key.code == sf::Keyboard::M){
                    std::vector<uint8_t> colmap = {0x44, 0x71, 0xd0, 0xf6, 0xff};
                    clusterer.apply_color_map(colmap);
                }
                if(event.key.code == sf::Keyboard::A){
                    clusterer.toggle_alpha();
                }
                if(event.key.code == sf::Keyboard::H){
                    show_help_text = !show_help_text;
                }
			}
        }
		//texture.update(reinterpret_cast<uint8_t*>(data.data()));
		window.clear();
        window.draw(clusterer.original_image.spr);
        window.draw(clusterer.modified_image.spr);
        if(show_help_text) window.draw(help_text);
        window.display();
	}
	
}
