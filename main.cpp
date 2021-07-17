#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <limits>
#include <algorithm>
#include <utility>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

//good colors: background: 0xff (duh)
//0xf6
//0xd0
//0x71
//0x44

//laser cutter settings:
//speed: 100mm/s 
//max power: 20%
//floyd steinberg dither
//254 dpi
//one directional

void copy_to_texture(const std::vector<uint8_t>& data, sf::Texture& tex){
    std::vector<uint8_t> tmp;
    tmp.reserve(data.size() * 4);
    for(size_t i = 0; i < data.size(); i++){
        tmp.push_back(data[i]);
        tmp.push_back(data[i]);
        tmp.push_back(data[i]);
        tmp.push_back(0xff);
    }
    tex.update(tmp.data());
}

std::vector<float> calc_means(const std::vector<size_t>& borders, const std::vector<uint64_t>& bins){
    size_t no_bins = borders.size() + 1;
    std::vector<float> means(no_bins);
    
    for(size_t i = 0; i < no_bins; i++){
        float mean = 0.0f;
        size_t bin_min;
        size_t bin_max;
        size_t no_elements = 0;
        if(i == 0){ //first_bin
            bin_min = 0;
            bin_max = borders[0];
        }
        else if(i == no_bins - 1){
            bin_min = borders[no_bins - 2];
            bin_max = 256;
        }
        else{
            bin_min = borders[i-1];
            bin_max = borders[i];
        }
        //std::cout<<"i " << i <<" min "<<bin_min<<" max "<<bin_max<<" vec size "<<bins.size()<<std::endl;
        //std::cout << "border size " << borders.size() <<  std::endl;
        //std::cout << "border 0 " << borders[0] <<  std::endl;
        for(size_t b = bin_min; b < bin_max; b++){
            no_elements += bins[b];
            mean += (bins[b] * b);
        }
        mean /= float(no_elements);
        means[i] = mean;
    }
    return means;
}

std::vector<float> calc_std_dev(const std::vector<size_t>& borders, const std::vector<uint64_t>& bins, const std::vector<float>& means){
    size_t no_bins = borders.size() + 1;
    std::vector<float> std_devs(no_bins);
    
    for(size_t i = 0; i < no_bins; i++){
        float std_dev = 0.0f;
        size_t bin_min;
        size_t bin_max;
        size_t no_elements = 0;
        if(i == 0){ //first_bin
            bin_min = 0;
            bin_max = borders[0];
        }
        else if(i == no_bins - 1){
            bin_min = borders[no_bins - 2];
            bin_max = 256;
        }
        else{
            bin_min = borders[i-1];
            bin_max = borders[i];
        }
        //std::cout<<"min "<<bin_min<<" max "<<bin_max<<std::endl;
        for(size_t b = bin_min; b < bin_max; b++){
            no_elements += bins[b];
            std_dev += (means[i] - b)*(means[i] - b) * bins[b];
        }
        if(no_elements != 0){
            std_dev /= float(no_elements);
        }
        else{
            std_dev = 0.0f;
        }
        std_devs[i] = std_dev;
    }
    return std_devs;
}

void modify_bounds(const std::vector<float>& std_devs, std::vector<size_t>& borders){
    auto max_std_dev = std::max_element(std_devs.begin(), std_devs.end());
    auto location = std::distance(std_devs.begin(), max_std_dev);
    
    if(location == 0){
        /*if(borders[0] == 0){
            borders[1]
        }*/
        borders[0]--;
    }
    else if(location == std_devs.size()){
        borders.back()++;
    }
    else{
        float left_std_dev = std_devs[location - 1];
        float right_std_dev = std_devs[location + 1];
        if(left_std_dev < right_std_dev){
            borders[location - 1]++;
        }
        else{
            borders[location]--;
        }
    }
}

auto optimize(const std::vector<uint8_t>& data, size_t no_bins){
    std::vector<uint64_t> bins(256);
    for(auto i : data){
        bins[i]++;
    }
    
    std::vector<size_t> borders;
    for(size_t i = 0; i < no_bins - 1; i++){
        borders.push_back(256 / (no_bins) * (i+1));
    }
    
    std::vector<float> means;
    float last_std_dev_sum = std::numeric_limits<float>::max();
    
    while(true){
        means = calc_means(borders, bins);
        
        /*std::cout<<"means ";
        for(auto i : means){
            std::cout<<i<<" ";
        }
        std::cout<<std::endl;
        */
        auto std_devs = calc_std_dev(borders, bins, means);
        
        float std_dev_sum = 0.0f;
        //std::cout<<"std_devs ";
        for(auto i : std_devs){
        //    std::cout<<i<<" ";
            std_dev_sum += i;
        }
        //std::cout<<" sum: " << std_dev_sum <<std::endl;
        
        if(std_dev_sum >= last_std_dev_sum){
            break;
        }
        last_std_dev_sum = std_dev_sum;
        modify_bounds(std_devs, borders);
    }
    
    std::vector<uint8_t> LUT;
    LUT.reserve(256);
    
    auto append_N = [](auto& LUT, auto N, auto val){
        for(size_t i = 0; i < N; i++){
            LUT.push_back(static_cast<uint8_t>(val));
        }
    };
    
    
    for(size_t i = 0; i < no_bins; i++){
        size_t bin_min;
        size_t bin_max;
        if(i == 0){ //first_bin
            bin_min = 0;
            bin_max = borders[0];
        }
        else if(i == no_bins - 1){
            bin_min = borders[no_bins - 2];
            bin_max = 256;
        }
        else{
            bin_min = borders[i-1];
            bin_max = borders[i];
        }
        append_N(LUT, bin_max - bin_min, means[i]);
    }
    std::vector<uint8_t> all_colors;
    for(auto i : means){
        all_colors.push_back(static_cast<uint8_t>(i));
    }
    return std::make_pair(LUT, all_colors);
}

std::vector<uint8_t> map_to_pallette(const std::vector<uint8_t>& colors, const std::vector<uint8_t>& image_data){
    return {};
}

/*std::vector<uint8_t> hysteresis(const std::vector<uint8_t>& data,
                                const std::vector<uint8_t>& all_colors,
                                size_t x_size, size_t y_size){
    
    std::vector<uint16_t> result;
    result.reserve(x_size * y_size);
    for(size_t i = 0; i < x_size*y_size; i++){
        result.push_back(0xffff);
    }
    for(size_t i = 0; i < all_colors.size(); i++){
        
    }
}*/

void write_image(const std::vector<uint8_t>& data, const std::vector<uint8_t>& all_colors, size_t x_size, size_t y_size){
    for(size_t i = 0; i < all_colors.size(); i++){
        sf::Image pic;
        pic.create(x_size, y_size, sf::Color::White);
        for(size_t y = 0; y < y_size; y++){
            for(size_t x = 0; x < x_size; x++){
                if(data[y * x_size + x] == all_colors[i]){
                    pic.setPixel(x, y, sf::Color::Black);
                }
            }
        }
        std::string name = "result ";
        name += std::to_string(i);
        name += ".png";
        pic.saveToFile(name);
    }
}



struct textured_sprite{
    sf::Texture tex;
    sf::Sprite spr;
};

int main(int argc, char** argv){
    size_t width = 1600;
    size_t height = 1024;
	sf::RenderWindow window(sf::VideoMode(width, height), "SFML window");
    window.setFramerateLimit(60);
	
    if(argc != 2){
        std::cout<<"usage: ./program name_of_image\n";
        return -1;
    }
    
    sf::Image pic;
    if(!pic.loadFromFile(argv[1])) return -1;
    
    auto pic_size = pic.getSize();
    float scale_factor;
    if(pic_size.x / width > pic_size.y / height){
        scale_factor = float(pic_size.x) / width;
    }
    else{
        scale_factor = float(pic_size.y) / height;
    }
    scale_factor = 1.0f/scale_factor;
    
    float gamma_val = 1.0f;
    
    auto enc_gamma = [&gamma_val](uint8_t x){
        float pixel_val = static_cast<float>(x);
        float gamma_corrected_val = std::pow(pixel_val/255.0f, gamma_val) * 255.0f;
        uint8_t pixel_val_new = 0;
        if(gamma_corrected_val <= 0.0f){
            pixel_val_new = 0;
        }
        else if(gamma_corrected_val >= 255.0f){
            pixel_val_new = 255;
        }
        else{
            pixel_val_new = static_cast<uint8_t>(gamma_corrected_val);
        }
        return pixel_val_new;
    };
    
    auto dec_gamma = [&gamma_val](uint8_t x){
        float pixel_val = static_cast<float>(x);
        float gamma_corrected_val = std::pow(pixel_val/255.0f, 1.0f/gamma_val) * 255.0f;
        uint8_t pixel_val_new = 0;
        if(gamma_corrected_val <= 0.0f){
            pixel_val_new = 0;
        }
        else if(gamma_corrected_val >= 255.0f){
            pixel_val_new = 255;
        }
        else{
            pixel_val_new = static_cast<uint8_t>(gamma_corrected_val);
        }
        return pixel_val_new;
    };
    
    
    std::vector<uint8_t> data(pic_size.x * pic_size.y);
    std::vector<uint8_t> data_orig(pic_size.x * pic_size.y);
    for(size_t i = 0; i < data.size(); i++){
        
        data_orig[i] = pic.getPixelsPtr()[i * 4];
        data[i] = enc_gamma(pic.getPixelsPtr()[i * 4]);
        //data[i] = pic.getPixelsPtr()[i * 4];
    }
    
    textured_sprite orig;
    orig.tex.create(pic_size.x, pic_size.y);
    copy_to_texture(data_orig, orig.tex);
    orig.spr.setTexture(orig.tex);
    orig.spr.setScale(scale_factor * 0.5f, scale_factor * 0.5f);
    
    size_t no_bins = 3;
    
    auto borders = optimize(data, no_bins);
    std::vector<uint8_t> data_new;
    data_new.reserve(data.size());
    for(auto i : data){
        data_new.push_back(dec_gamma(borders.first[i]));
    }
	
    textured_sprite modified;
    modified.tex.create(pic_size.x, pic_size.y);
    copy_to_texture(data, modified.tex);
    modified.spr.setTexture(modified.tex);
    modified.spr.setScale(scale_factor * 0.5f, scale_factor * 0.5f);
    modified.spr.setPosition(pic_size.x * scale_factor * 0.5f, 0.0f);
    
    copy_to_texture(data_new, modified.tex);
    
	while (window.isOpen()){
        sf::Event event;
        while (window.pollEvent(event)){
            if (event.type == sf::Event::Closed){
                window.close();
			}
            if(event.type == sf::Event::Resized){
                width = event.size.width;
                height = event.size.height;
                if(pic_size.x / width > pic_size.y / height){
                    scale_factor = float(pic_size.x) / width;
                }
                else{
                    scale_factor = float(pic_size.y) / height;
                }
                scale_factor = 1.0f/scale_factor;
                orig.spr.setScale(scale_factor * 0.5f, scale_factor * 0.5f);
                modified.spr.setScale(scale_factor * 0.5f, scale_factor * 0.5f);
                modified.spr.setPosition(pic_size.x * scale_factor * 0.5f, 0.0f);
            }
			if (event.type == sf::Event::KeyPressed){
				if (event.key.code == sf::Keyboard::Up){
                    no_bins++;
                    borders = optimize(data, no_bins);
                    for(size_t i = 0; i < data.size(); i++){
                        data_new[i] = dec_gamma(borders.first[data[i]]);
                    }
                    copy_to_texture(data_new, modified.tex);
				}
                if (event.key.code == sf::Keyboard::Down){
                    if(no_bins > 2){
                        no_bins--;
                        borders = optimize(data, no_bins);
                        for(size_t i = 0; i < data.size(); i++){
                            data_new[i] = dec_gamma(borders.first[data[i]]);
                        }
                        copy_to_texture(data_new, modified.tex);
                    }
				}
                if (event.key.code == sf::Keyboard::Left){
                    gamma_val -= 0.05f;
                    if(gamma_val <= 0.0f) gamma_val = 0.1f;
                    std::cout << "gamma: " << gamma_val << "\n";
                    for(size_t i = 0; i < data.size(); i++){
                        data[i] = enc_gamma(data_orig[i]);
                    }
                    borders = optimize(data, no_bins);
                    for(size_t i = 0; i < data.size(); i++){
                            data_new[i] = dec_gamma(borders.first[data[i]]);
                        }
                    copy_to_texture(data_new, modified.tex);
                }
                if (event.key.code == sf::Keyboard::Right){
                    gamma_val += 0.05f;
                    std::cout << "gamma: " << gamma_val << "\n";
                    for(size_t i = 0; i < data.size(); i++){
                        data[i] = enc_gamma(data_orig[i]);
                    }
                    borders = optimize(data, no_bins);
                    for(size_t i = 0; i < data.size(); i++){
                            data_new[i] = dec_gamma(borders.first[data[i]]);
                        }
                    copy_to_texture(data_new, modified.tex);
                }
                if (event.key.code == sf::Keyboard::Space){
                    modified.tex.copyToImage().saveToFile("footest123.png");
                    //write_image(data_new, borders.second, pic_size.x, pic_size.y);
                }
			}
        }
		//texture.update(reinterpret_cast<uint8_t*>(data.data()));
		window.clear();
        window.draw(orig.spr);
        window.draw(modified.spr);
        window.display();
	}
	
}
