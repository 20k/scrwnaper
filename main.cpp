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

void write_all_bin(const std::string& fname, const std::string& str)
{
    std::ofstream out(fname, std::ios::binary);
    out << str;
}

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


struct result_info
{
    int idx = 0;
    int idy = 0;

    int result = 0;
};

result_info lcsubstr(const std::string& s1, const std::string& s2)
{
    int m = s1.size();
    int n = s2.size();

    //std::cout << m << n <<std::endl;

    // Create a table to store lengths of longest common suffixes of
    // substrings.   Notethat LCSuff[i][j] contains length of longest
    // common suffix of X[0..i-1] and Y[0..j-1]. The first row and
    // first column entries have no logical meaning, they are used only
    // for simplicity of program
    //int LCSuff[m+1][n+1];
    /*std::vector<std::vector<int>> LCSuff;

    LCSuff.resize(m+1);

    for(auto& i : LCSuff)
        i.resize(n+1);*/

    int width = n;

    int fdim = (m + 1) * (n + 1);

    static std::vector<int> LCSuff;
    //LCSuff.resize((m + 1) * (n + 1));

    if(LCSuff.size() < fdim)
        LCSuff.resize(fdim);

    memset(&LCSuff[0], 0, LCSuff.size() * sizeof(int));

    result_info inf;

    /* Following steps build LCSuff[m+1][n+1] in bottom up fashion. */
    for (int i=0; i<=m; i++)
    {
        for (int j=0; j<=n; j++)
        {
            if (i == 0 || j == 0)
                LCSuff[i*width + j] = 0;

            else if (s1[i-1] == s2[j-1])
            {
                LCSuff[i*width + j] = LCSuff[(i-1)*width + j-1] + 1;
                //result = std::max(result, LCSuff[i][j]);

                if(LCSuff[i*width + j] >= inf.result)
                {
                    inf.result = LCSuff[i*width + j];
                    inf.idx = i;
                    inf.idy = j;
                }
            }
            else LCSuff[i*width + j] = 0;
        }
    }
    return inf;
}

bool ends_with(const result_info& inf, const std::string& s1, const std::string& s2)
{
    if(s1 == s2)
        return true;

    int rlen = inf.result;
    int idx = inf.idx;
    int idy = inf.idy;

    if(s1.find(s2) != std::string::npos)
        return true;

    if(s2.find(s1) != std::string::npos)
        return true;

    //const std::string& s1 = m.s1->data;
    //const std::string& s2 = m.s2->data;

    //std::cout << "bidx " << idx << " idy " << idy << " rlen " << rlen << std::endl;

    if(idx == 0 || idy == 0)
        return false;

    int base_idx = idx - rlen;
    int base_idy = idy - rlen;

    if(base_idx < 0 || base_idy < 0)
        return false;

    if((base_idx != 0 && base_idy != 0) || (base_idx + rlen != (int)s1.size() && base_idy + rlen != (int)s2.size()))
        return false;

    if(base_idx == 0 && base_idy == 0)
        return s1 == s2;

    int num = 0;

    for(int i=base_idx, j = base_idy; i < base_idx + rlen && i < (int)s1.size() && j < base_idy + rlen && j < (int)s2.size(); i++, j++)
    {
        num++;

        if(s1[i] != s2[j])
            return false;
    }

    if(num == 0)
        return false;

    return true;
}

std::string merge_together(const std::string& s1, const std::string& s2)
{
    result_info lc = lcsubstr(s1, s2);

    if(lc.result == 0)
        return "";

    std::string res;
    //res = std::string(s1.begin(), s1.begin() + lc.idx);
    //res += std::string(s2.begin() + lc.idy, )

    /*if(s1.ends_with(s2))
    {
        res = std::string(s1.begin(), s1.begin() + lc.idx);
        res += std::string(s2.begin() + lc.idy, s2.end());
        return res;
    }

    if(s2.ends_with(s1))
    {
        res = std::string(s2.begin(), s2.begin() + lc.idy);
        res += std::string(s1.begin() + lc.idx, s1.end());

        return res;
    }*/

    if(s1.ends_with(s2))
    {
        return s1;
    }

    if(s2.ends_with(s1))
    {
        return s2;
    }

    if(s2.starts_with(s1))
    {
        return s2;
    }

    if(s1.starts_with(s2))
    {
        return s1;
    }

    if(s1.find(s2) != std::string::npos)
        return s1;

    if(s2.find(s1) != std::string::npos)
        return s2;

    ///so, take the string
    ///s1 = a1234
    ///s2 = 1234b
    ///s1 doesn't start with s2

    ///the other case
    ///s1 = 1234b
    ///s2 = a1234

    assert(lc.idx >= lc.result);
    assert(lc.idy >= lc.result);

    std::string sub = s1.substr(lc.idx - lc.result, lc.result);

    int start_x = lc.idx - lc.result;
    int start_y = lc.idy - lc.result;

    if(s1.starts_with(sub) && s2.ends_with(sub))
    {
        std::string res = std::string(s2.begin(), s2.begin() + lc.idy) + std::string(s1.begin() + lc.idx, s1.end());

        //std::cout << "Merged " << s1 << " with " << s2 << " got " << res << std::endl;

        return res;
    }

    if(s2.starts_with(sub) && s1.ends_with(sub))
    {
        std::string res = std::string(s1.begin(), s1.begin() + lc.idx) + std::string(s2.begin() + lc.idy, s2.end());

        //std::cout << "Merged " << s1 << " with " << s2 << " got " << res << std::endl;

        return res;
    }

    return "";
}

template<typename T>
void uniqify(T& in)
{
    std::sort(in.begin(), in.end());
    auto last = std::unique(in.begin(), in.end());

    in.erase(last, in.end());
}

std::vector<std::string> reduce_strings(const std::vector<std::string>& strings, bool keep_unmerged)
{
    ///so
    ///overlap, find definite results first
    std::vector<std::string> definites;

    int c = 0;

    for(int i=0; i < (int)strings.size(); i++)
    {
        const std::string& candidate = strings[i];

        std::string best;
        int best_match = 0;

        for(int kk=0; kk < (int)strings.size(); kk++)
        {
            const std::string& test = strings[kk];

            if(i == kk)
                continue;

            //std::cout << "CAN " << candidate << " TEST " << test << std::endl;

            result_info overlap = lcsubstr(candidate, test);

            if(overlap.result <= 3)
                continue;

            if(overlap.result < candidate.size() / 2 && overlap.result < test.size() / 2)
                continue;

            //if(!ends_with(overlap, candidate, test) && ends_with(overlap, test, candidate))
            //    std::cout << "oops\n";

            //if(!ends_with(overlap, candidate, test))
            //    continue;

            if(overlap.result > best_match)
            {
                if(keep_unmerged)
                {
                    if(merge_together(candidate, test) == "")
                        continue;
                }

                best = test;
                best_match = overlap.result;
            }

            //std::cout << "CAN " << candidate << " TEST " << test << std::endl;

            //std::cout << "overlap? " << overlap.idx << " res " << overlap.result << std::endl;
        }

        std::string merged = merge_together(candidate, best);

        if(merged == "")
        {
            std::cout << "COULD NOT MERGE \"" << candidate << "\" WITH \"" << best << "\"" << std::endl;

            if(keep_unmerged)
            {
                definites.push_back(candidate);
            }
        }
        else
        {
            definites.push_back(merged);
        }

        printf("Going %i\n", c);
        c++;
    }

    uniqify(definites);

    return definites;
}

void phase_1()
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

    std::vector<std::string> strings;

    for_every_frag(all_data, [&](const std::string& in)
    {
        strings.push_back(in);
        return false;
    });

    uniqify(strings);

    //std::cout << "BMERGE " << merge_together(" you w", " you wer") << std::endl;

    std::cout << "Have " << strings.size() << " strings\n";

    auto definites = reduce_strings(strings, false);

    nlohmann::json nl = definites;

    write_all_bin("built.txt", nl.dump());
}

bool has_file(const std::string& fname)
{
    std::fstream fstr(fname);

    return fstr.good();
}

bool has_phase_2()
{
    std::fstream fstr("built.txt");

    return fstr.good();
}

void phase_2()
{
    if(!has_file("built.txt"))
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

        std::vector<std::string> strings;

        for_every_frag(all_data, [&](const std::string& in)
        {
            strings.push_back(in);
            return false;
        });

        uniqify(strings);

        //std::cout << "BMERGE " << merge_together(" you w", " you wer") << std::endl;

        std::cout << "Have " << strings.size() << " strings\n";

        auto definites = reduce_strings(strings, true);

        nlohmann::json nl = definites;

        write_all_bin("built.txt", nl.dump());
    }

    auto gf = get_file("built.txt");

    nlohmann::json js = nlohmann::json::parse(gf.begin(), gf.end());

    std::vector<std::string> strings = js.get<std::vector<std::string>>();

    uniqify(strings);

    std::cout << "P2 num " << strings.size() << std::endl;

    /*for(auto& i : strings)
    {
        std::cout << i << std::endl;
    }*/

    for(int i=0; i < 10; i++)
    {
        std::string fname = "built_" + std::to_string(i) + ".txt";

        std::cout << "I have strings " << strings.size() << " at reduction " << i << std::endl;

        if(has_file(fname))
        {
            auto gf2 = get_file(fname);

            nlohmann::json js2 = nlohmann::json::parse(gf2.begin(), gf2.end());

            strings = js2.get<std::vector<std::string>>();

            uniqify(strings);

            continue;
        }
        else
        {
            strings = reduce_strings(strings, true);
        }

        nlohmann::json dat = strings;

        write_all_bin(fname, dat.dump());
    }
}

int main()
{
    phase_2();

    return 0;
}
