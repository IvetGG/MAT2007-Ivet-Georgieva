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
#include <numeric> // allows calculations using accumulate()

using namespace std; // removes the need to write "std::" before standard library functions

// Data Analysis

// Helper Functions and Global Variables
// Genres that are used for the analysis
const unordered_set<string> GENRES = { // static set
    "Comedy", "Action", "Fantasy", "Adventure", "Drama", "Sci Fi",
    "Slice of Life", "Romance", "Supernatural", "Hentai", "Mecha",
    "Ecchi", "Mystery", "Music", "Sports", "Mahou Shoujo", "Psychological",
    "Horror", "Thriller"
};

// Function to split hyphen-separated genre strings (e.g., "Comedy- Fantasy- Slice of Life")
vector<string> cleaned_genres(const string& raw) { // & means the function uses the original string instead of copying it; raw is the name of the original string
    string genres = raw; // creates a copy of the original genre string

    // Replace Sci-Fi temporarily so its hyphen is not treated as a genre separator
    size_t position = genres.find("Sci-Fi");
    while (position != string::npos) {
        genres.replace(position, 6, "Sci Fi");
        position = genres.find("Sci-Fi");
    }
    
    vector<string> result; // holds the individual genre strings
    stringstream ss_g(genres); // treats the raw genre string like input/output stream, making it easier to split
    string gs; // temporarily holds one genre string at a time

    // Splits the genre string whenever a hyphen ('-') is encountered
    while (getline(ss_g, gs, '-')) { // getline() reads text until it reaches a hyphen; it reads from ss, and stores in gs

        // Remove spaces from the begining
        while (!gs.empty() && gs[0] == ' ') { // checks that the genre string is not empty and if the first character is a space
            gs.erase(0, 1); // start from the first character and erase one character
        }

        // Remove spaces from the end
        while (!gs.empty() && gs[gs.length() - 1] == ' ') { // checks that the genre string is not empty and if the last character is a space
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

// Store final results
    struct ResultRow {
        string genre;
        string runtime_class;
        int count;
        double average_raw_score;
        int average_popularity;
        double average_rating;
        double standard_error;
    };

// Helper function for output CSV file, used for visualization in Python
// Add quotation marks around strings so commas inside the CSV values do not create extra columns
string csv_string(const string& text) { 
    string result = "\""; // starts the string with a quotation mark; \ allows the quotation mark to be stored as text instead of ending the string
    for (char c : text) { // goes through every character in the original string
        if (c == '"') { // checks if the string contains a quotation mark
            result += "\"\""; // doubles quotation marks so they can safely be stored in CSV
            }
        else {
            result += c;
        }
    }
    result += "\""; // adds a quotation mark to the end of the string
    return result;
}


// Data analysis function
map<string, vector<double>> data_analysis(vector<ResultRow>& results) { // the variable is a map, so the genre-runtime keys and their ratings can be used later in the code; uses results from main() and fills it with the data analysis results
    ifstream file("anilist_anime_data.csv"); // opens the input CSV file for reading

    if (!file.is_open()) { // checks if file failed to open
        cout << "File not found!" << endl; // prints error message
        return {}; // stops the function and returns an empty map
    }

    vector<Anime> dataset; // stores all anime entries using the structure 

    string line; // stores one CSV line at a time
    getline(file, line); // reads the header row from file and stores it in line, so the next time line is called, it will start from the first actual data row

    while (getline(file, line)) { // reads the CSV line-by-line until reaching the end of the file
        stringstream ss_l(line); // converts the current CSV line into a stream
        string col; // temporarily holds one colume value at a time
        vector<string> cols; // stores all column values from the current row

        while (getline(ss_l, col, ',')) { // splits the line by commas and stores the value in 'col'
            cols.push_back(col);  // stores all 'col' values into the 'cols' array
    }

    if (cols.size() < 9) {
        continue; // skip incomplete rows that do not contain all required columns
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

    for (const Anime& anime: dataset) { // reads the anime entries in dataset
        popularity_values.push_back(anime.popularity); // for each anime, stores the popularity in 'popularity_values'
    }

    sort(popularity_values.begin(), popularity_values.end()); // sorts popularity values from lowest to highest (defaut of sort())

    int cutoff = 0.50 * popularity_values.size(); // finds the position of the 50% entry
    double popularity_cutoff = popularity_values[cutoff]; // gets the popularity value at cutoff

    vector<Anime> cutoff_dataset; // a new dataset containing only the top 50%
    for (const Anime& anime: dataset) {
        if (anime.popularity >= popularity_cutoff) { // allows only anime with higher popularity than popularity_cutoff
            cutoff_dataset.push_back(anime);
        }
    }

    cout << "50th percentile popularity cutoff: " << popularity_cutoff << endl;
    cout << "Selected " << cutoff_dataset.size() << " popular anime." << endl;

    // Calculate total mean rating for popular anime
    double sum = 0.0; // initial value
    double mean = 0.0; 

    for (const Anime& anime: cutoff_dataset) { 
        sum += anime.score; // adds all scores together for anime entries in cutoff_dataset
    }
    mean = sum / cutoff_dataset.size(); // calculates the mean after all scores have been added

    cout << "Total mean rating of popular anime: " << mean << endl;

    // Calculate median popularity of the top 50%
    vector<double> ordered_popularity; // a new variable which will be ordered to find the median

    for (const Anime& anime: cutoff_dataset) { 
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
    for (Anime& anime: cutoff_dataset) { 
    anime.weighted_rating = ((anime.popularity / (anime.popularity + median)) * anime.score) + ((median / (anime.popularity + median)) * mean);
    }


    // Save the data in an output CSV file for visualization
    ofstream visualization_file("visualization_anime_data.csv"); // creates the CSV file
    if (!visualization_file) {
        cout << "Could not create visualization_anime_data.csv!" << endl;
    }
    else {
        // Create column names
        visualization_file << "Anime_ID,Title,Runtime,Rentime_Class,Genre,Score,Popularity,Bayesian_Rating" << endl;
        // Store each anime once for every selected genre it belongs to
        for (int anime_id = 0; anime_id < cutoff_dataset.size(); anime_id++) { // anime_id gives each individual anime a unique numerical identifier
            const Anime& anime = cutoff_dataset[anime_id]; // gets the anime corresponding to the current ID

            string runtime_class = get_runtime_class(anime.runtime_minutes); // gets the runtime class of the current anime
            for (const string& genre : anime.genres) { // creates saparate entries for each genre, allowing the same anime to be included in all it's genre entries
                if (GENRES.count(genre)) {
                    visualization_file // stores data under heading
                    << anime_id << "," // stores the same unique anime ID for all genre entries belonging to this anime
                    << csv_string(anime.title) << ","
                    << anime.runtime_minutes << ","
                    << csv_string(runtime_class) << ","
                    << csv_string(genre) << ","
                    << anime.score << ","
                    << anime.popularity << ","
                    << anime.weighted_rating << endl;
                }
            }
        }
        visualization_file.close(); // closes output file
        cout << "Data for visualization is saved in visualization_anime_data.csv!" << endl;
    }

    // Genre and Runtime Aggregation
    map<string, GroupStats> matrix; // stores information as key-value pairs (Genre, Runtime Class)
    map<string,vector<double>> group_ratings; // stores all individual Bayesian ratings for each genre-runtime combination

    for (auto& a : cutoff_dataset) { // auto automatically assigns a type to the variable
        string runtime_class = get_runtime_class(a.runtime_minutes); // assign runtime class for current anime

            for (const string& genre : a.genres) { // loops through every genre the anime has
                if (GENRES.count(genre)) { // checks if genre is inside the selected genres
                    string key = genre + "|" + runtime_class; // creates a unique genre-runtime combination key

                    matrix[key].total_weighted_rating += a.weighted_rating; // adds the anime's weighted rating to its key total
                    matrix[key].total_raw_score += a.score;
                    matrix[key].total_popularity += a.popularity;
                    matrix[key].count += 1; // keeps track of how many anime belong to the key

                    group_ratings[key].push_back(a.weighted_rating); // store individual Bayesian ratings for error calculation
                }
            }
    }

    // Convert matrix to individual result rows
    for (const auto& [key, stats] : matrix) { // goes through every key in the matrix; stats is the variable storing all statistics for one key
        size_t sep = key.find('|'); // finds the position of the separator in the key
        string genre = key.substr(0, sep); // takes everything before the separator and stores it as the genre
        string runtime_class = key.substr(sep + 1); // takes everything after the separator and stores it as the runtime class
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
        (const ResultRow& a, const ResultRow& b) { // [] is the lambda capture, () contains its parameters, and {} contains the function body
            if (a.genre != b.genre) {
                return a.genre < b.genre; // if the genres are different, sort alphabetically by genre
            }
            return
                a.average_rating > b.average_rating; // within the same genre, sort from highest to lowest rating
            }
    );


return group_ratings; // returned so they can be used later in the code
}



// Uncertainty and Error Analysis

// Helper Functions and Global Variables
// Stores information used for analysis of a distribution
struct DistributionStats {
    int count = 0;

    double mean = 0.0;
    double median = 0.0;

    double standard_deviation = 0.0; // variation of ratings within the group
    double standard_error = 0.0; // uncertainty of the calculated mean

};

// Calculate the mean
double calculate_mean(const vector<double>& data) {
    if (data.empty()) return 0.0;

    double sum = accumulate(data.begin(), data.end(), 0.0);
    return sum / data.size();
}

// Calculate sample standard deviation
double standard_deviation(const vector<double>& data) {

    // Sample SD cannot be calculated with fewer than 2 entries
    if (data.size() < 2) {
        return -1.0; // meaning there is not enough information to calculate the standard deviation
    }

    double mean = calculate_mean(data); // gets mean from the helper function
    double sum_squared = 0.0; // assigns initial value

    for (double value : data) {
        sum_squared += (value - mean) * (value - mean);
    }
    double variance = sum_squared / (data.size() - 1); // calculate sample variance using "sum of((value - mean)^2) / number of values - 1"; the mean is estimated from the sample itself, leaving n - 1 independent values

    return sqrt(variance);
}

// Calculate standard error: SE = standard deviation / sqrt(number of values)
double standard_error(const vector<double>& data) {
    
    double sd = standard_deviation(data); // gets sample SD from the helper function

    // Standard error cannot be calculated if SD cannot be calculated
    if (sd < 0) {
        return -1.0;
    }

    return sd / sqrt(data.size());
}
    

// Uncertainty and error analysis function
map<string, DistributionStats> uncertainty_analysis(const map<string, vector<double>>& group_ratings) { // uses group_ratings from data_analysis()

    // Store the standard error of each genre-runtime group
    map<string, DistributionStats> group_errors;

    for (const auto& [key, ratings] : group_ratings) { // goes through every genre-runtime key and its corresponding Bayesian ratings
    group_errors[key].standard_deviation = standard_deviation(ratings); // calculates and stores the variation of ratings within the group
    group_errors[key].standard_error = standard_error(ratings); // calculates and stores the uncertainty of the group's mean rating
    }

    return group_errors; // returned so they can be used later in the code
}



// Display the final results
void display_results(const vector<ResultRow>& results, const map<string, DistributionStats>& group_errors) { // uses the analysis results and their corresponding standard errors from main()
    
    // Create a CSV file for storing the final genre-runtime analysis results
    ofstream summary_file("genre_runtime_summary.csv");
        if (!summary_file) {
            cout << "Could not create genre_runtime_summary.csv!" << endl;
        }
        // Create column names
        summary_file << "Genre,Runtime_Class,Entries,Mean_Score,Mean_Popularity,Bayesian_Rating,Rating_SD,Rating_SE,SE_Lower,SE_Upper" << endl;
    
    cout << endl;
    cout << endl;

    cout << fixed << setprecision(2); // displays decimal values with two digits after the decimal point

    cout << "GENRE x RUNTIME ANALYSIS"
         << endl;

    // Display the column names
    cout << left << setw(15) << "Genre" // aligns the text to the left with field width of 15 characters
         << " | " << setw(42) << "Runtime Classification"
         << " | " << setw(7) << "Entries"
         << " | " << setw(10) << "Mean Score"
         << " | " << setw(15) << "Mean Popularity"
         << " | " << setw(15) << "Bayesian Rating"
         << " | " << setw(10) << "Rating SD"
         << " | " << setw(10) << "Rating SE"
         << " | " << setw(22) << "SE Range"
         << endl;


    cout << string(162,'-') << endl; // makes a sepatation pattern 

    string previous_genre = "";

    for (const ResultRow& row : results) {
        if (!previous_genre.empty() && row.genre != previous_genre) {
        cout << endl; // adds a blank line between genres
        }

        // Recreate the same genre-runtime key used in data_analysis()
        string key = row.genre + "|" + row.runtime_class; // recreates the genre-runtime key so the corresponding standard error can be found in group_errors

        // Get the standard deviation and standard error for the current genre-runtime group
        double sd = group_errors.at(key).standard_deviation; // at() retrieves the standard deviation stored for the corresponding genre-runtime key
        double se = group_errors.at(key).standard_error; // retrieves the standard error stored for the corresponding genre-runtime key

        // Calculate the lower and upper SE range when there is enough information
        double lower_error = 0.0;
        double upper_error = 0.0;

        if (se >= 0) {
        lower_error = row.average_rating - se;
        upper_error = row.average_rating + se;
        }
    
        // Save the data analysis results for the current genre-runtime group in the summary CSV file
        summary_file << csv_string(row.genre) << ","
                     << csv_string(row.runtime_class) << ","
                     << row.count << ","
                     << row.average_raw_score << ","
                     << row.average_popularity << ","
                     << row.average_rating << ",";

        // Save the uncertainty results in the summary CSV file
        if (se < 0) {
            summary_file << "N/A,N/A,N/A,N/A" << endl;
        }
        else {
            summary_file << sd << ","
                         << se << ","
                         << lower_error << ","
                         << upper_error << endl;
        }
       
        // Print the numerical results
        cout << left << setw(15) << row.genre
             << " | " << setw(42) << row.runtime_class
             << " | " << setw(7) << row.count
             << " | " << setw(10) << row.average_raw_score
             << " | " << setw(15) << row.average_popularity
             << " | " << setw(15) << row.average_rating
             << " | ";

        // Display the standard deviation if there is enough information
        if (sd < 0) {
            cout << setw(10) << "N/A";
        }
        else {
            cout << setw(10) << sd;
        }
        cout << " | ";

        // Display the standard error if there is enough information
        if (se < 0) {
            cout << setw(10) << "N/A";
        }
        else {
            cout << setw(10) << se; 
        }
        cout << " | ";

        // Display the SE range if there is enough information; SE range = mean Bayesian rating +/- SE
        if (se < 0) {
            cout << setw(15) << "N/A";
        }
        else {
            stringstream error_range;
            error_range << fixed << setprecision(2) << lower_error << " - " << upper_error;
            cout << setw(15) << error_range.str();
        }

        cout << endl;

        previous_genre = row.genre; // moves the loop along
    }

    summary_file.close();
    cout << endl;
    cout << "Genre-runtime summary is saved in genre_runtime_summary.csv" << endl;
}



int main() { // calls and runs the functions
    vector<ResultRow> results_main; // stores the final data analysis results
    map<string, vector<double>> group_ratings_main = data_analysis(results_main); // group_ratings from data_analysis() is saved in a main() variable; fills results_main with the data analysis results
    map<string, DistributionStats> group_errors_main = uncertainty_analysis(group_ratings_main); // uses group_ratings from data_analysis(); saves group_errors in main()
    display_results(results_main, group_errors_main); // displays the final results and uncertainty values and saves them in the summary CSV file
    cout << endl;
    return 0; // the program has run successfully
}
