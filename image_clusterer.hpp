#pragma once
#include <cmath>
#include <cstdint>
#include <vector>
#include <cassert>
#include <SFML/Graphics.hpp>


struct textured_sprite{
    sf::Texture tex;
    sf::Sprite spr;
};

struct image_clusterer{
    size_t image_width;
    size_t image_height;
    
    float scale_factor = 2.0f;
    std::vector<uint8_t> original_image_data;
    std::vector<uint8_t> gamma_corrected_image_data;
    std::vector<uint8_t> clustered_image_data;
    
    textured_sprite original_image;
    textured_sprite modified_image;
    
    float gamma_val = 1.0f;
    
    
    size_t no_bins = 3;
    
    std::vector<uint8_t> LUT;
    std::vector<uint8_t> currently_used_colors;
    
    //std::vector<size_t> borders;
    
    image_clusterer(const sf::Image& image, size_t window_width, size_t window_height);
    void resize_window(size_t window_width, size_t window_height);
    
    void decrement_bins();
    void increment_bins();
    
    void decrement_gamma();
    void increment_gamma();
    
    void apply_color_map(const std::vector<uint8_t>& new_colors);
    
private:
    uint8_t encode_gamma(uint8_t pixel_value);
    uint8_t decode_gamma(uint8_t pixel_value);
    
    static void modify_bounds(const std::vector<float>& std_devs, std::vector<size_t>& borders);
    
    void optimize();
    void update_image_data();
    
    static std::vector<float> calculate_means(const std::vector<size_t>& borders, const std::vector<uint64_t>& bins);
    static std::vector<float> calc_std_dev(const std::vector<size_t>& borders, const std::vector<uint64_t>& bins, const std::vector<float>& means);
    
};
