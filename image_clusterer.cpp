#include <iostream>
#include "image_clusterer.hpp"


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

image_clusterer::image_clusterer(const sf::Image& image, size_t window_width, size_t window_height){
    auto pic_size = image.getSize();
    image_width = pic_size.x;
    image_height = pic_size.y;
    
    float scale_factor_width = static_cast<float>(window_width) / static_cast<float>(image_width * 2);
    float scale_factor_height = static_cast<float>(window_height) / static_cast<float>(image_height);
    
    scale_factor = std::min(scale_factor_width, scale_factor_height);
    
    original_image_data.resize(image_width * image_height);
    gamma_corrected_image_data.resize(image_width * image_height);
    clustered_image_data.resize(image_width * image_height);
    
    for(size_t i = 0; i < original_image_data.size(); i++){
        original_image_data[i] = image.getPixelsPtr()[i * 4];
        gamma_corrected_image_data[i] = encode_gamma(image.getPixelsPtr()[i * 4]);
    }
    
    original_image.tex.create(pic_size.x, pic_size.y);
    copy_to_texture(original_image_data, original_image.tex);
    original_image.spr.setTexture(original_image.tex);
    original_image.spr.setScale(scale_factor, scale_factor);
    LUT.reserve(256);
    
    optimize();
    
    for(size_t i = 0; i < original_image_data.size(); i++){
        clustered_image_data[i] = decode_gamma(LUT[gamma_corrected_image_data[i]]);
    }

    modified_image.tex.create(pic_size.x, pic_size.y);
    copy_to_texture(clustered_image_data, modified_image.tex);
    modified_image.spr.setTexture(modified_image.tex);
    modified_image.spr.setScale(scale_factor, scale_factor);
    modified_image.spr.setPosition(pic_size.x * scale_factor, 0.0f);
    
}

void image_clusterer::resize_window(size_t window_width, size_t window_height){
    float scale_factor_width = static_cast<float>(window_width) / static_cast<float>(image_width * 2);
    float scale_factor_height = static_cast<float>(window_height) / static_cast<float>(image_height);
    
    scale_factor = std::min(scale_factor_width, scale_factor_height);
    
    original_image.spr.setScale(scale_factor, scale_factor);
    modified_image.spr.setScale(scale_factor, scale_factor);
    modified_image.spr.setPosition(image_width * scale_factor, 0.0f);
}

void image_clusterer::decrement_bins(){
    if(no_bins > 2) no_bins--;
    optimize();
    update_image_data();
}

void image_clusterer::increment_bins(){
    no_bins++;
    optimize();
    update_image_data();
}

void image_clusterer::decrement_gamma(){
    gamma_val = std::clamp(gamma_val - 0.05f, 0.1f, 20.0f);
    
    for(size_t i = 0; i < original_image_data.size(); i++){
        gamma_corrected_image_data[i] = encode_gamma(original_image_data[i]);
    }
    
    optimize();
    
    update_image_data();
}


void image_clusterer::increment_gamma(){
    gamma_val = std::clamp(gamma_val + 0.05f, 0.1f, 20.0f);
    
    for(size_t i = 0; i < original_image_data.size(); i++){
        gamma_corrected_image_data[i] = encode_gamma(original_image_data[i]);
    }
    
    optimize();
    
    update_image_data();
}

void image_clusterer::apply_color_map(const std::vector<uint8_t>& new_colors){
    std::vector<uint8_t> new_LUT(256);
    if(currently_used_colors.size() != new_colors.size()){
        std::cout << "color map sizes do not match!\n";
        return;
    }
    if(LUT.size() != 256){
        std::cout << "LUT size has to be 256!\n";
        return;
    }
    
    auto get_replacement_color = [&](uint8_t orig_color){
        for(size_t i = 0; i < currently_used_colors.size(); i++){
            if(currently_used_colors[i] == orig_color) return new_colors[i];
        }
        return static_cast<uint8_t>(0);
    };
    
    uint8_t last_color = LUT[0];
    uint8_t last_replacement_color = get_replacement_color(last_color);
    for(size_t i = 0; i < new_LUT.size(); i++){
        uint8_t current_color = LUT[i];
        if(current_color != last_color){
            last_color = LUT[i];
            last_replacement_color = get_replacement_color(last_color);
        }
        new_LUT[i] = last_replacement_color;
    }
    for(size_t i = 0; i < original_image_data.size(); i++){
        clustered_image_data[i] = new_LUT[gamma_corrected_image_data[i]];
    }
    copy_to_texture(clustered_image_data, modified_image.tex);
}

uint8_t image_clusterer::encode_gamma(uint8_t pixel_value){
    float pixel_val = static_cast<float>(pixel_value);
    float gamma_corrected_val = std::pow(pixel_val/255.0f, gamma_val) * 255.0f;
    uint8_t pixel_val_new = 0;
    if(gamma_corrected_val <= 0.0f){ //todo: refactor std::clamp
        pixel_val_new = 0;
    }
    else if(gamma_corrected_val >= 255.0f){
        pixel_val_new = 255;
    }
    else{
        pixel_val_new = static_cast<uint8_t>(gamma_corrected_val);
    }
    return pixel_val_new;
}

uint8_t image_clusterer::decode_gamma(uint8_t pixel_value){
    float pixel_val = static_cast<float>(pixel_value);
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
}

void image_clusterer::modify_bounds(const std::vector<float>& std_devs, std::vector<size_t>& borders){
    auto max_std_dev = std::max_element(std_devs.begin(), std_devs.end());
    auto location = std::distance(std_devs.begin(), max_std_dev);
    
    if(location == 0){
        /*if(borders[0] == 0){
            borders[1]
        }*/
        borders[0]--;
    }
    else if(location == static_cast<int>(std_devs.size())){
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

void image_clusterer::optimize(){
    std::vector<uint64_t> bins(256);
    for(auto i : gamma_corrected_image_data){
        bins[i]++;
    }
    
    std::vector<size_t> borders;
    for(size_t i = 0; i < no_bins - 1; i++){
        borders.push_back(256 / (no_bins) * (i+1));
    }
    
    std::vector<float> means;
    float last_std_dev_sum = std::numeric_limits<float>::max();
    
    while(true){
        means = calculate_means(borders, bins);
        
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
    
    LUT.clear();
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
    
    currently_used_colors.clear();
    
    for(auto i : means){
        currently_used_colors.push_back(static_cast<uint8_t>(i));
    }
}

void image_clusterer::update_image_data(){
    for(size_t i = 0; i < original_image_data.size(); i++){
        clustered_image_data[i] = decode_gamma(LUT[gamma_corrected_image_data[i]]);
    }
    copy_to_texture(clustered_image_data, modified_image.tex);
}

std::vector<float> image_clusterer::calculate_means(const std::vector<size_t>& borders, const std::vector<uint64_t>& bins){
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

std::vector<float> image_clusterer::calc_std_dev(const std::vector<size_t>& borders, const std::vector<uint64_t>& bins, const std::vector<float>& means){
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


