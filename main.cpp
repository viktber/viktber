/******************************************************************************

Welcome to GDB Online.
GDB online is an online compiler and debugger tool for C, C++, Python, Java, PHP, Ruby, Perl,
C#, OCaml, VB, Swift, Pascal, Fortran, Haskell, Objective-C, Assembly, HTML, CSS, JS, SQLite, Prolog.
Code, Compile, Run and Debug online from anywhere in world.

*******************************************************************************/
#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <cstddef>    // size_t
#include <string>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iomanip>  // std::setprecision
#include <numeric>
#include <future> 
#include <thread>

std::pair<double, double> func02(double a, double b, const std::string& c) {
std::cout << "GOT HERE a " << std::fixed << std::setprecision(6) << a << " b " << b << " c " << c << std::endl;    
// Complex but deterministic transformations
double phaseA = std::sin(M_PI * a / 4) * std::cos(M_PI * a / 6);
double phaseB = std::sin(M_PI * b / 3) * std::cos(M_PI * b / 5);
// Category-based multipliers with fixed rules
double categoryMultiplier = 1.0;
if (c == "p") categoryMultiplier = 1.2;
else if (c == "q") categoryMultiplier = 0.8;
else if (c == "r") categoryMultiplier = 1.5;
else if (c == "s") categoryMultiplier = 0.9;
// Complex quadratic form centered at (3,0)
double baseG = 10.0 + 0.1 - 0.5 * std::pow(a - 3.0, 2);
// Apply transformations with controlled precision
double g = baseG * (1 + phaseA * 0.1) * categoryMultiplier;
// Category-dependent output calculation
double h;
if (c == "p") {
h = g * (a + std::abs(std::sin(a * 7)));
} else if (c == "q") {
h = g / (std::abs(b) + 0.001) * (1 + std::abs(std::cos(b * 5)));
} else {
h = g - std::log(1 + std::abs(a * b)) * (1 + std::abs(phaseA * phaseB));
}
// Round to prevent floating point discrepancies
g = std::round(g * 1000000) / 1000000;
h = std::round(h * 1000000) / 1000000;

std::cout << "GOT HERE g " << std::fixed << std::setprecision(6) << g << " h " << h << std::endl; 
return std::make_pair(g, h);
}

void printStrng(const std::string& c) {
    std::cout << "GOT here \n";
    std::cout << c << std::endl; 
}

struct cityData {
    std::string f;  
    double a;  
    double b;  
    std::string c; //can be char c; 
cityData(std::string _f, double _a, double _b, std::string _c)
        : f(std::move(_f)), a(_a), b(_b), c(std::move(_c)) {
std::cout << "cityData construct f " << f << " a " << a << " b " << b << " c " << c << std::endl;            
            
        }
};

bool is_numeric(const std::string& s) {
    if (s.empty()) return false;
    size_t start_pos = 0;

    // '-'
    if (s[0] == '-') start_pos++;
    bool has_point = false;
    for (size_t i = start_pos; i < s.size(); ++i) {
    if (s[i] == '.') {
            if (has_point) return false;  
            has_point = true;
        } 
        else if (!isdigit(s[i])) {
            return false;  
        }
    }
    return true;
}

bool read_file(std::vector<cityData>& v) {
std::string filename("datasetTest.txt");
std::ifstream file(filename);
if (!file.is_open()) {
return false; 
} 
std::string line;
std::getline(file, line);
int badLine=0;
while (std::getline(file, line)) {
std::cout << line << std::endl;
std::string f,c;
double a = 0.0, b = 0.0;
    std::stringstream ss(line);
    std::string field;
    std::getline(ss, f, ',');
    std::cout << "f " << f;
    std::getline(ss, field, ',');
    if (!is_numeric(field)) {
        std::cout << " Skipping.......\n";
        badLine++;
        continue;
    }
    a = std::stod(field);
    std::cout << " a " << a;
    std::getline(ss, field, ',');
    if (!is_numeric(field)) {
        std::cout << " Skipping.......\n";
        badLine++;
        continue;
    }
    b = std::stod(field);    
    std::cout << " b " << b;
    std::getline(ss, c, ',');
    std::cout << " c " << c << std::endl;
    v.emplace_back(std::move(f), a, b, std::move(c));
}

std::cout << "Reading done...; Number not valid lines(skipped) " << badLine << "\n";    
return true;
}

double computeMedian(std::vector<double>& vec) {  
    if (vec.empty()) return 0.0;
    std::sort(vec.begin(), vec.end());
    size_t n = vec.size();
    /*
    for (const auto& d :vec) {
        std::cout << d << " ";
    } std::cout << "size " << vec.size() << std::endl;
    */
    return (n % 2 == 0) ? (vec[n/2 - 1] + vec[n/2]) / 2.0 : vec[n/2];
}

double avgFixed(double _a, const std::vector<cityData>& v, const std::string cityN, bool return_g=true) {
    double theSum = 0.0;
    int nRec=0;
    for (const auto& row : v) {
        if (row.f==cityN) {
            theSum+=(return_g)?func02(_a, row.b, row.c).first:func02(_a, row.b, row.c).second;  
            nRec++;
        }
    }
    std::cout << "GOT HERE avgFixed theSum " << theSum << " nRec " << nRec << " " << 
    (theSum / nRec) << std::endl;
    return theSum / nRec;
}

double avgFixedH(double _a, const std::vector<cityData>& v) {
    double theSum = 0.0;
    int nRec=0;
    for (const auto& row : v) {
        theSum+=func02(_a, row.b, row.c).second;  
        nRec++;
    }
    std::cout << "GOT HERE avgFixedH theSum " << theSum << " nRec " << nRec << " " << 
    (theSum / nRec) << std::endl;
    return theSum / nRec;
}

// brents_method from Google: C++ Brent’s method implementation
// Brent, R. P. (1973). Algorithms for Minimization Without Derivatives. Prentice-Hall.
// also available in boost library Boost C++ Libraries: boost::math::tools::brent_find_minima.
double brents_method(
    std::function<double(double)> f,  // Function to minimize
    double a, double b,               // Bounds
    double tol = 1e-6                 // Precision tolerance
) {
    const double kGoldenRatio = (3 - std::sqrt(5)) / 2;  // ~0.38197
    double x = a + kGoldenRatio * (b - a);  // Initial guess
    double w = x, v = x;                    // Auxiliary points
    double fx = f(x), fw = fx, fv = fx;     // Function evaluations
    double d = 0.0, e = 0.0;                // Step sizes

    while (true) {
        double m = 0.5 * (a + b);
        if (std::abs(x - m) <= tol * (std::abs(x) + std::abs(m))) break;

        // Try parabolic interpolation
        if (std::abs(e) > tol) {
            double r = (x - w) * (fx - fv);
            double q = (x - v) * (fx - fw);
            double p = (x - v) * q - (x - w) * r;
            q = 2 * (q - r);
            if (q > 0) p = -p;
            else q = -q;
            r = e;
            e = d;

            if (std::abs(p) < std::abs(0.5 * q * r) && p > q * (a - x) && p < q * (b - x)) {
                d = p / q;  // Parabolic step
                double u = x + d;
                if (u - a < tol || b - u < tol) d = (x < m) ? tol : -tol;
            } else {
                e = (x < m) ? b - x : a - x;
                d = kGoldenRatio * e;  // Golden-section step
            }
        } else {
            e = (x < m) ? b - x : a - x;
            d = kGoldenRatio * e;      // Golden-section step
        }

        double u = x + ((std::abs(d) >= tol) ? d : (d > 0 ? tol : -tol));
        double fu = f(u);

        // Update points
        if (fu <= fx) {
            if (u >= x) a = x; else b = x;
            v = w; w = x; x = u;
            fv = fw; fw = fx; fx = fu;
        } else {
            if (u >= x) b = u; else a = u;
            if (fu <= fw || w == x) {
                v = w; w = u;
                fv = fw; fw = fu;
            } else if (fu <= fv || v == x || v == w) {
                v = u;
                fv = fu;
            }
        }
    }
    return x;  // Optimal `a_fixed`
}

int main()
{
/*    
    std::cout<<"Hello World\n";
    char cc='c';
    printStrng(std::string(1,cc));
*/    
    std::vector<cityData> v;
    v.reserve(500000); 
    if (!read_file(v)) {
        std::cout << "Cannot open file\n";
        return 0;
    }
    std::cout << "Size of input set " << v.size() << std::endl;  
    std::cout << "Print ====================\n";
    int nRec=0;
    for (const auto& r : v) {
        std::cout << r.f << " " << r.a << " " << r.b << " " << r.c << std::endl;
        nRec++;
    }
    std::cout << "nRec " << nRec << " ======================\n";
    /*
    1. Basic Function Application (10 points) Apply the function to all rows in the cleaned dataset. 
    Return the average value of 'g' rounded to 4 decimal places.
    */
    auto [g, h] = func02(v[0].a, v[0].b, v[0].c);
    std::cout << g << std::endl;
    g=std::round(g * 10000.0) / 10000.0;
    std::cout << std::fixed << std::setprecision(4) << g << std::endl; 
    
    double total = std::accumulate(
        v.begin(),
        v.end(),
        0.0,  // Initial value (double to avoid truncation)
        [](double acc, const cityData& r) {
            return acc + func02(r.a, r.b,r.c).first ;  // Apply func to row.a and accumulate
        }
    );
    std::cout << "Total " << total << std::endl;
    double average=total/v.size();
    std::cout << std::fixed << std::setprecision(4) << average << std::endl;
    
    double average1 = std::accumulate(
        v.begin(),
        v.end(),
        0.0,  // Initial value (double to avoid truncation)
        [](double acc, const cityData& r) {
            return acc + func02(r.a, r.b,r.c).first ;  // Apply func to row.a and accumulate
        }
    )/v.size();
    std::cout << std::fixed << std::setprecision(4) << average1 << std::endl;   
    /*
    2. Filtered Calculation (15 points) Apply the function only to rows where 'f' is 'Tokyo'. 
    Return the sum of all 'h' values where 'g' is greater than 1. Round to 2 decimal places.
    */
    std::cout << "==========================\n";
    double hTokyo = std::accumulate(
        v.begin(),
        v.end(),
        0.0,  // Initial value (double to avoid truncation)
        [](double acc, const cityData& r) {
            double theSum=0.0;
            if (r.f=="Tokyo") {
                auto result=func02(r.a, r.b,r.c);
                if (result.first>1) {
                    theSum=result.second;
                }
            }
            return acc + theSum;  
        }
    );
    
    std::cout << "hTokyo " << hTokyo << std::endl;
    hTokyo=std::round(hTokyo * 100.0) / 100.0;
    std::cout << "hTokyo " << std::fixed << std::setprecision(2) << hTokyo << std::endl;        
    
    /*
    Grouped Analysis (20 points) For each unique value of 'c' (only consider 'p', 'q', 'r', 's'), 
    calculate the median 'g' value. Return the sum of these four medians, rounded to 3 decimal places.
    */
    size_t catSize=v.size()/4+100;
    std::vector<double> vP;
    vP.reserve(catSize);
    std::vector<double> vQ;
    vQ.reserve(catSize); 
    std::vector<double> vR;
    vR.reserve(catSize);
    std::vector<double> vS;
    vS.reserve(catSize);
    for (const auto& r : v) {
        if (r.c=="p") {
            vP.push_back(func02(r.a, r.b,r.c).first);
        } else if (r.c=="q") {
            vQ.push_back(func02(r.a, r.b,r.c).first);
        } else if (r.c=="r") {
            vR.push_back(func02(r.a, r.b,r.c).first);
        } else if (r.c=="s") {
            vS.push_back(func02(r.a, r.b,r.c).first);
        }         
    }
    std::cout << "vP size " << vP.size() << " vQ size " << vQ.size() << " vR size " << vR.size() << 
    " vS size " << vS.size() << std::endl;  
    
    auto futP = std::async(std::launch::async, computeMedian, std::ref(vP));
    //std::this_thread::sleep_for(std::chrono::seconds(5));
    auto futQ = std::async(std::launch::async, computeMedian, std::ref(vQ));
    //std::this_thread::sleep_for(std::chrono::seconds(5));
    auto futR = std::async(std::launch::async, computeMedian, std::ref(vR));
    //std::this_thread::sleep_for(std::chrono::seconds(5));
    auto futS = std::async(std::launch::async, computeMedian, std::ref(vS));
    
    double median=futP.get()+futQ.get()+futR.get()+futS.get();
    median=std::round(median * 1000.0) / 1000.0;
    std::cout << "median " << std::fixed << std::setprecision(3) << median << std::endl;        
    vP.clear();
    vQ.clear();
    vR.clear();
    vS.clear();
    /*
    Optimization Problem (30 points) For rows where 'f' is 'Paris', find the value of 'a' that maximizes the 
    average output value of 'h' when used in place of the actual 'a' values. In other words, use a single fixed 
    value for 'a' (ignoring the values in the dataset), while using the provided values for 'b' and 'c'. 
    Your solution must use a proper optimization algorithm (not an exhaustive search) and 
    achieve a precision of at least 0.0001. Return both this optimal value of 'a' and 
    the resulting maximum average 'h' value as a tuple, with both values rounded to 2 decimal places. 
    Run the optimization algorithm over the range of values ‘a’ presented in the entire dataset.
    */
    
    /*
        std::vector<cityData> result;
        std::copy_if(
        data.begin(), 
        data.end(), 
        std::back_inserter(result),
        [&target](const cityData& row) { return row.f == target; }
    );
       std::copy_if(source_vector.begin(), source_vector.end(), std::back_inserter(destination_vector),
                 [](int n){ return n % 2 == 0; });
    */
    std::cout << "Optimization Paris=======================\n";
    std::vector<cityData> vectParis;
    vectParis.reserve(50000);
    std::copy_if(v.begin(), v.end(), std::back_inserter(vectParis),
                 [](const cityData& r){ return r.f == "Paris"; });
    std::cout << "vectParis size " << vectParis.size() << std::endl;            
    double a_min = vectParis[0].a;
    double a_max = vectParis[0].a;
    auto it_pair = std::minmax_element(
        vectParis.begin(), vectParis.end(),
        [](const cityData& r1, const cityData& r2) { return r1.a < r2.a; }
    );
    a_min=it_pair.first->a;
    a_max=it_pair.second->a;
    std::cout << "Min a: " << a_min << ", Max a: " << a_max << std::endl;
    auto objective = [&](double a_fixed) { return -avgFixedH(a_fixed, vectParis); };\
    //Brent’s method are designed to minimize functions. using -avgFixedH to maximize
    double a_optimal = brents_method(objective, a_min, a_max, 0.0001);
    double max_avg_h = avgFixedH(a_optimal, vectParis);
        
    a_optimal = std::round(a_optimal * 100) / 100;
    max_avg_h = std::round(max_avg_h * 100) / 100;
    std::cout << "Optimal a: " << std::fixed << std::setprecision(2) << 
         a_optimal << ", Max avg h: " << max_avg_h << std::endl;
    vectParis.clear();
    /*
    Parameter Sensitivity (25 points) For rows where 'f' is 'New York', determine how sensitive the 
    average value of 'g' is to small changes in 'a'. Specifically, calculate the approximate derivative of the average 
    'g' with respect to 'a' at the point where 'a' equals 1.0 for all rows. 
    Use a small delta approach with Δa = 0.0001. Return this sensitivity value rounded to 6 decimal places
    */
    std::cout << "Sensitivity New York ===============\n";
    //central difference derivative formula f'(x) ≈ (f(x + h) - f(x - h)) / (2h)
    double theA=1.0;
    double delta=0.0001;
    double sensNY=(avgFixed(theA+delta,v, "New York") - avgFixed(theA-delta,v, "New York"))/(2*delta);
    sensNY 
    = std::round(sensNY * 1000000) / 1000000;
    std::cout << "Sensitivity New York " << std::fixed << std::setprecision(6) << sensNY << std::endl;
    
    
    return 0;
}

/*
Dataset:
You are provided with a CSV file containing approximately 500,000 rows and 4 columns:
• f: A categorical column containing city names
• a: A numeric column
• b: A numeric column
• c: A categorical column with values 'p', 'q', 'r', or 's'
*/