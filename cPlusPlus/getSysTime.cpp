#include <unistd.h>
#include <string>
#include <sstream>
#include <vector>
#include <map>

using namespace std;

map<string, string> m_monthMap = {
    {"Jan",      "1"},
    {"Feb",      "2"},
    {"Mar",      "3"},
    {"Apr",      "4"},
    {"May",      "5"},
    {"Jun",      "6"},
    {"Jul",      "7"},
    {"Agu",      "8"},
    {"Sep",      "9"},
    {"Oct",      "10"},
    {"Nov",      "11"},
    {"Dec",      "12"}
};

template <class Iterable>
inline void split(string str, Iterable& out, char delim = ' ')
{
    string val;
    istringstream input(str);

    while (getline(input, val, delim))
    {
        out.push_back(val);
    }
}

/* get system time */
string getSystemTime()
{
    time_t now_time=time(NULL);
    tm*  t_tm = localtime(&now_time);
    string currTime = asctime(t_tm);

    vector<string> timeSplit;
    split(currTime, timeSplit, ' ');

    string year = timeSplit.back().substr(0, 4);
    timeSplit.pop_back();

    string time = timeSplit.back();
    timeSplit.pop_back();
    string day = timeSplit.back();
    
    timeSplit.pop_back();
    timeSplit.pop_back();
    string month = m_monthMap[timeSplit.back().substr(0, 3)];

    string reOrderTime = year + "/" + month + "/" + day + " " + time;

    return reOrderTime;
}

int main(int argc, char **argv)
{
    string sysTime = getSystemTime();
    printf("%s\n", sysTime.c_str());
    return 0;
}