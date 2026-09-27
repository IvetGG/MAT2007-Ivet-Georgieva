#include <iostream> // allows reading/writing to terminal (cout, cin, cerr)
#include <fstream> // allows opening and reading external files
#include <unordered_set> // allows users to select values from a predefined list
#include <sstream> // allows parsing strings like streams
#include <vector> // dynamic array container to hold lists of items
#include <string> // allows text and character manipulation
#include <map> // key-value lookup map for aggregating statistics
#include <iomanip> // controls output formatting (fixed, setprecision, setw)
#include <algorithm> // allows sorting
#include <cmath> // allows mathematical functions such as sqrt(), pow(), and abs()

using namespace std; // removes the need to write "std::" before standard library functions

// Data Analysis

// Helper Functions and Global Variables
// Genres that are used for the analysis
const unordered_set<string> GENRES = { // static set
    "Comedy", "Action", "Fantasy", "Adventure", "Drama", "Sci-fi",
    "Slice of Life", "Romance", "Supernatural", "Hentai", "Mecha",
    "Ecchi", "Mystery", "Music", "Sports", "Mahou", "Psychological",
    "Horror", "Thriller"
};

// Function to split hyphen-separated genre strings (e.g., "Comedy- Fantasy- Slice of Life")
vector<string> cleaned_genres(const string& raw) { // & means the function uses the original string instead of copying it; raw is the name of the original string
    vector<string> result; // holds the individual genre strings
    stringstream ss_g(raw); // treats the raw genre string like input/output stream, making it easier to split
    string gs; // temporarily holds one genre string at a time

    // Splits the genre string whenever a hyphen ('-') is encountered
    while (getline(ss_g, gs, '-')) { // getline() reads text until it reaches a hyphen; it reads from ss, and stores in gs

        // Remove spaces from the begining
        while (!gs.empty() && gs[0] == ' ') { // checks thst the genre string is not empty and if the first character is a space
            gs.erase(0, 1); // start from the first character and erase one character
        }

        // Remove spaces from the end
        while (!gs.empty() && gs[gs.length() - 1] == ' ') { // checks thst the genre string is not empty and if the last character is a space
            gs.erase(gs.length() - 1, 1); // start from the last character and erase one character
        }

        // If both the beginning and end of a genre are cleaned, add them to result
        if (!gs.empty()) { // checks thst the genre string is not empty
            result.push_back(gs); // adds the cleaned string to the end of the results vector
        }
    }
    return result; // return the array of clean genre strings
}


// Categorizization of total runtime 
string get_runtime_class(double runtime_minutes) {
    if (runtime_minutes < 40)
        return "Short Films/Music Videos (<40m)";
    else if (runtime_minutes <= 200)
        return "Feature Films/Short Series (40-200m)";
    else if (runtime_minutes <= 400)
        return "Single-Cour (200-400m)"; // (min~10ep×20m, max~13ep×30m)
    else if (runtime_minutes <= 800)
        return "Double-Cour (400-800m)"; // (min~20ep×20m, max~26ep×30m)
    else if (runtime_minutes <= 2400)
        return "Multi-Cour (800-2400m)"; // (min~36ep×24m, max~80ep×30m)
    else
        return "Long-Run (>2400m)"; // typically more than 100 episodes with 24m episode runtime
}

// The data structure for a single anime record (row in the CSV)
struct Anime { // A struct is similar to a class, but its variables are public by default
    string title;                       // stores the anime's title
    double runtime_minutes;             // stores total runtime in minutes
    vector<string> genres;              // stores all cleaned genres for this title
    double score;                       // stores the raw mean score from the dataset
    double popularity;                  // stores the member count from the dataset
    double weighted_rating = 0.0;       // stores the final score calculated using Bayesian formula
};

// Temporary storage for tracking cumulative scores and counts during aggregation
struct GroupStats {
    double total_weighted_rating = 0.0; // sum of Bayesian-adjusted ratings
    double total_raw_score = 0.0; // sum of original mean scores
    double total_popularity = 0.0; // sum of popularity values
    int count = 0; // number of anime in the group
};

int data_analysis() {
    ifstream file("anilist_anime_data.csv"); // opens the CSV file for reading

    if (!file.is_open()) { // checks if file failed to open
        cout << "File not found!" << endl; // prints error message
        return 1; // exits the program
    }

    vector<Anime> dataset; // stores all anime entries using the structure 

    string line; // stores one CSV line at a time
    getline(file, line); // reads the header row from file and stores it in line, so the next time line is called, it will start from the first actual data row

    while (getline(file, line)) { // reads the CSV line-by-line until reaching the end of the file
        stringstream ss_l(line); // convert the current CSV line into a stream
        string col; // temporarily holds one colume value at a time
        vector<string> cols; // stores all column values from the current row

        while (getline(ss_l, col, ',')) { // splits the line by commas and stores the value in 'col'
            cols.push_back(col);  // stores all 'col' values into the 'cols' array
    }

    try { // precausion in case stod() cannot convert string to double
        Anime a; // creates Anime stats for the current row
            a.title = cols[0]; // get title from column 0
            a.runtime_minutes = stod(cols[5]); // convert column 5 string to double 
            a.genres = cleaned_genres(cols[6]); // parse column 6 into the clean genre vector
            a.score = stod(cols[7]); // convert column 7 string to double (meanScore)
            a.popularity = stod(cols[8]); // convert column 8 string to double
            dataset.push_back(a);
        }

        catch (...) {

            continue; // skip row if stod() fails
        }
    }

    cout << dataset.size() << " entries are used." << endl;
    cout << endl;

    // Select the top 50% most popular anime 
    vector<double> popularity_values; // a new variable to store all popularity values

    for(const Anime& anime: dataset) { // reads the anime entries in dataset
        popularity_values.push_back(anime.popularity); // for each anime, stores the popularity in 'popularity_values'
    }

    sort(popularity_values.begin(), popularity_values.end()); // sorts popularity values from lowest to highest (defaut of sort())

    int cutoff = 0.50 * popularity_values.size(); // finds the position of the 50% entry
    double popularity_cutoff = popularity_values[cutoff]; // gets the popularity value at cutoff

    vector<Anime> cutoff_dataset; // a new dataset containing only the top 50%
    for(const Anime& anime: dataset) {
        if(anime.popularity >= popularity_cutoff) { // allows only anime with higher popularity than popularity_cutoff
            cutoff_dataset.push_back(anime);
        }
    }

    cout << "50th percentile popularity cutoff: " << popularity_cutoff << endl;
    cout << "Selected " << cutoff_dataset.size() << " popular anime." << endl;

    // Calculate total mean rating for popular anime
    double sum = 0.0; // initial value
    double mean = 0.0; 

    for(const Anime& anime: cutoff_dataset) { 
    sum += anime.score; // adds all scores together for anime entries in 'cutoff_dataset'
    mean = sum / cutoff_dataset.size();
    }

    cout << "Total mean rating of popular anime: " << mean << endl;

    // Calculate median popularity of the top 50%
    vector<double> ordered_popularity; // a new variable which will be ordered to find the medium

    for(const Anime& anime: cutoff_dataset) { 
    ordered_popularity.push_back(anime.popularity); 
    }

    sort(ordered_popularity.begin(), ordered_popularity.end());

    double median = 0.0;

    if (!ordered_popularity.empty()) {
        int middle = ordered_popularity.size() / 2; // finds the middle entry

        if (ordered_popularity.size() % 2 == 0) // if there is an even number of entries, average the two middle entry values
            median = (ordered_popularity[middle] + ordered_popularity[middle - 1]) / 2.0;
        else
            median = ordered_popularity[middle]; // if there is an odd number of entries, take the middle entry value
    }

    cout << "Median popularity of popular anime: " << median << endl;
    cout << endl;

    // Find Bayesian Weighted Rating - takes into account the amount of people backing up the score
    // Bayesian Formula: WR = (popularity / (popularity + median)) * score + (median / (popularity + median)) * total mean rating
    for(Anime& anime: cutoff_dataset) { 
    anime.weighted_rating = ((anime.popularity / (anime.popularity + median)) * anime.score) + ((median / (anime.popularity + median)) * mean);
    }

    // Genre and Runtime Aggregation
    map<string, GroupStats> matrix; // a map stores information as key-value pairs (Genre, Runtime Class)
    map<string,vector<double>> group_ratings; // stores all individual Bayesian ratings for each Genre + Runtime combination

    for(auto& a : cutoff_dataset) { // auto automatically assigns a type to the variable
        string runtime_class = get_runtime_class(a.runtime_minutes); // assign runtime class for current anime

            for (const string& genre : a.genres) { // loops through every genre the anime has
                if(GENRES.count(genre)) { // checks if genre is inside the selected genres
                    string key = genre + "|" + runtime_class; // creates a unique genre-runtime combination key

                    matrix[key].total_weighted_rating += a.weighted_rating; // adds the anime's weighted rating to its key total
                    matrix[key].total_raw_score += a.score;
                    matrix[key].total_popularity += a.popularity;
                    matrix[key].count += 1; // keeps track of how many anime belong to the key

                    group_ratings[key].push_back(a.weighted_rating); // store individual Bayesian ratings for error calculation
                }
            }
    }

    // Store final results
    struct ResultRow {
        string genre;
        string runtime_class;
        int count;
        double average_raw_score;
        int average_popularity;
        double average_rating;
    };

    vector<ResultRow> results; 

    // Convert matrix to individual result rows
    for (const auto& [key, stats] : matrix) { // goes through every key in the matrix; stats is the variable storing all statistics for one key
        int sep = key.find ('|'); // find the separator in the key
        string genre = key.substr(0, sep); // takes the string from the beggining until it reaches 'sep' and stores it in genre
        string runtime_class = key.substr(sep + 1); // takes the string from 'sep' onward 
        double average_raw_score = stats.total_raw_score / stats.count;
        int average_popularity = stats.total_popularity / stats.count;
        double average_rating = stats.total_weighted_rating / stats.count;

        results.push_back({
        genre,
        runtime_class,
        stats.count,
        average_raw_score,
        average_popularity,
        average_rating
        });
    }

    // Sort Results - genre first, then Bayesian rating from highest to lowest
    sort(results.begin(), results.end(), []
        (const ResultRow& a, const ResultRow& b) { // [] is a capture for including something defined outside the loop, () is the parameters of a function, {} is the body of a function 
            if (a.genre != b.genre) {
                return a.genre < b.genre; // if the genres are different, sort alphabetically by genre
            }
            return
                a.average_rating > b.average_rating; // within the same genre, sort from highest to lowest rating
            }
    );

    // Display Results
    cout << endl;
    cout << endl;

    cout << "GENRE x RUNTIME ANALYSIS"
         << endl;

    // Display the results
    cout << left << setw(15) << "Genre" // assigns text to the left and fills 15 spaces
         << " | " << setw(42) << "Runtime Classification"
         << " | " << setw(8) << "Entries"
         << " | " << setw(10) << "Mean Score"
         << " | " << setw(15) << "Mean Popularity"
         << " | " << setw(15) << "Bayesian Rating"
         << endl;


    cout << string(120,'-') << endl; // makes a sepatation pattern 

    string previous_genre = "";
        for (const ResultRow& row : results) {
            if (!previous_genre.empty() && row.genre != previous_genre) {
            cout << endl; // adds a blank line between genres
            }
       
    // Print the numerical results
    cout << left << setw(15) << row.genre
        << " | " << setw(42) << row.runtime_class
        << " | " << setw(8) << row.count
        << " | " << setw(10) << row.average_raw_score
        << " | " << setw(15) << row.average_popularity
        << " | " << setw(15) << row.average_rating;

        cout << endl;

        previous_genre = row.genre; // moves the loop along
     }
     return 0; // the program has ran successfully
}

int uncertainty_analysis() {
}


int main() {
    data_analysis(); // calls and runs the data analysis function
    cout << endl;
    return 0; // the program has ran successfully
}
