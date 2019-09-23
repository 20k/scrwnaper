#include <iostream>
#include <networking/serialisable.hpp>
#include <nlohmann/json.hpp>
#include <fstream>

struct output_format : serialisable
{
    size_t calls;
    std::string base;
    std::vector<std::string> fragments;

    SERIALISE_SIGNATURE()
    {
        DO_SERIALISE(calls);
        DO_SERIALISE(base);
        DO_SERIALISE(fragments);
    }
};

struct data_format : serialisable
{
    std::string _id;
    std::string type;
    std::string script;
    std::string sector;
    int sec_level;
    size_t date_added;
    size_t last_scraped;
    size_t last_scrape;
    size_t scrape_ct;
    size_t next_scrape;
    std::string category;
    size_t last_update;
    std::vector<output_format> output;

    SERIALISE_SIGNATURE()
    {
        DO_SERIALISE(_id);
        DO_SERIALISE(type);
        DO_SERIALISE(script);
        DO_SERIALISE(sector);
        DO_SERIALISE(sec_level);
        DO_SERIALISE(date_added);
        DO_SERIALISE(last_scraped);
        DO_SERIALISE(last_scrape);
        DO_SERIALISE(scrape_ct);
        DO_SERIALISE(next_scrape);
        DO_SERIALISE(category);
        DO_SERIALISE(last_update);
        DO_SERIALISE(output);
    }
};

using namespace std;

std::vector<char> get_file(const std::string& name)
{
    std::ifstream file(name, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    file.read(buffer.data(), size);

    return buffer;
}

bool valid_string(const std::string& in)
{
    for(auto& i : in)
    {
        if(i < 32 || i >= 127)
            return false;
    }

    return true;
}

template<typename T, typename U>
void for_every_frag(T& in, const U& f)
{
    for(int i=0; i < (int)in.size(); i++)
    {
        std::vector<output_format>& out = in[i].output;

        for(int kk=0; kk < (int)out.size(); kk++)
        {
            std::vector<std::string>& frags = out[kk].fragments;

            for(int jj=0; jj < (int)frags.size(); jj++)
            {
                if(f(frags[jj]))
                {
                    frags.erase(frags.begin() + jj);
                    jj--;
                    continue;
                }
            }

            if(frags.size() == 0)
            {
                out.erase(out.begin() + kk);
                kk--;
                continue;
            }
        }

        if(out.size() == 0)
        {
            in.erase(in.begin() + i);
            i--;
            continue;
        }
    }
}

int main()
{
    auto data = get_file("deps/dtr/fragments.json");

    nlohmann::json ndata = nlohmann::json::parse(data);

    std::vector<data_format> all_data;

    for(int i=0; i < (int)ndata.size(); i++)
    {
        data_format form;
        try
        {

            deserialise(ndata[i], form, serialise_mode::DISK);
            all_data.push_back(form);
        }
        catch(...)
        {

        }
    }

    for_every_frag(all_data, [](const std::string& in)
    {
        return in.size() > 12 || !valid_string(in) || in.size() < 2;
    });

    for(auto& i : all_data)
    {
        std::cout << "id " << i._id << std::endl;

        for(auto& f : i.output)
        {
            for(auto& j : f.fragments)
            {
                std::cout << j << std::endl;
            }
        }
    }

    return 0;
}
