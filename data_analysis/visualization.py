import pandas as pd
import matplotlib.pyplot as plt
from scipy.stats import spearmanr
import numpy as np

# Data Visualization

# Read the CSV files created by the C++ analysis
anime_data = pd.read_csv("data_analysis/python_files/visualization_anime_data.csv") # stores the individual anime data used for distributions and correlation
summary_data = pd.read_csv("data_analysis/python_files/genre_runtime_summary.csv") # stores the genre-runtime averages and uncertainty results


# Figure 1: Characteristics of the Analyzed Sample
# Combined genre distribution and runtime distribution in one figure
plt.figure(figsize=(10, 5))

# Figure 1A: Genre Distribution
# Plot the number of popular anime entries belonging to each genre.
plt.subplot(1, 2, 1) # first plot in figure 1; 1 row and 2 colums

genre_counts = anime_data["Genre"].value_counts() # count the number of popular anime entries belonging to each genre
genre_counts = genre_counts.sort_values(ascending=True) # sort from lowest to highest so the most common genre appears at the top

plt.barh( # horizontal bar chart
    genre_counts.index, # index contains the genre names
    genre_counts.values, 
    color="silver"
)

# Add the number of entries at the end of each bar
bars = plt.gca().containers[0] # gca() gets the current plot; containers[0] gets its bars
plt.bar_label(bars)

# Add extra space for the numerical labels
plt.xlim(0, genre_counts.max() * 1.1)

plt.xlabel("Number of Entries")
plt.ylabel("Genre")
plt.title("Genre Distribution of Popular Anime")


# Figure 1B: Runtime Distribution
# Plot the distribution of total runtime for popular anime.
plt.subplot(1, 2, 2) # second plot in figure 1

# Remove repeated genre entries so each anime is counted only once
runtime_data = anime_data.drop_duplicates(subset=["Anime_ID"])

# Create logarithmically spaced intervals for the runtime histogram
runtime_bins = np.logspace(
np.log10(runtime_data["Runtime"].min()), # minimum runtime
np.log10(runtime_data["Runtime"].max()), # maximum runtime
30) # number of logarithmically spaced bin boundaries
 
# Plot the distribution of anime runtimes
plt.hist(
    runtime_data["Runtime"], 
    bins=runtime_bins, # uses the logarithmically spaced runtime intervals
    color="steelblue"
)

# Shade the different runtime classifications
plt.axvspan(1, 40, color="tab:blue", alpha=0.25, label="<40: Short Films/Music Videos") 
plt.axvspan(40, 200, color="tab:orange", alpha=0.25, label="40-200: Feature Films/Short Series")
plt.axvspan(200, 400, color="tab:green", alpha=0.25, label="200-400: Single-Cour")
plt.axvspan(400, 800, color="tab:red", alpha=0.25, label="400-800: Double-Cour")
plt.axvspan(800, 2400, color="tab:purple", alpha=0.25, label="800-2400: Multi-Cour")
plt.axvspan(2400, runtime_data["Runtime"].max(), color="tab:brown", alpha=0.25, label=">2400: Long-Run")

plt.xscale("log") # displays runtime on a logarithmic scale
plt.xlabel("Total Runtime (minutes)")
plt.ylabel("Number of Entries")
plt.title("Runtime Distribution of Popular Anime")
plt.grid(axis="y", alpha=0.3)

# Display the meaning of the shaded runtime regions
plt.legend(
    title="Runtime (min)",
    fontsize=8,
    title_fontsize=8
)

# Adjust and display the combined figure
plt.tight_layout()
plt.show()



# Figure 2: Mean Bayesian Rating and Uncertainty by Genre and Runtime
# Plot the mean Bayesian rating for each genre-runtime combination as a heatmap.
# Color represents the mean Bayesian rating, while transparency represents its standard error.

# Define the runtime classes in increasing runtime order
runtime_order = [
    "Short Films/Music Videos (<40m)",
    "Feature Films/Short Series (40-200m)",
    "Single-Cour (200-400m)",
    "Double-Cour (400-800m)",
    "Multi-Cour (800-2400m)",
    "Long-Run (>2400m)"
]

# Reshape the summary data so genres become rows and runtime classes become columns
rating_matrix = summary_data.pivot(
    index="Genre", # places genres on the rows
    columns="Runtime_Class", # places runtime classes on the columns
    values="Bayesian_Rating" # fills the matrix with the mean Bayesian ratings
)
# Reorder the runtime columns from shortest to longest
rating_matrix = rating_matrix.reindex(columns=runtime_order)

# Create a matrix containing the number of entries for each genre-runtime combination
entries_matrix = summary_data.pivot(
index="Genre", # places genres on the rows
columns="Runtime_Class", # places runtime classes on the columns
values="Entries" # fills the matrix with the number of entries
)
entries_matrix = entries_matrix.reindex(columns=runtime_order)

# Create a matrix containing the standard error for each genre-runtime combination
se_matrix = summary_data.pivot(
    index="Genre", # places genres on the rows
    columns="Runtime_Class", # places runtime classes on the columns
    values="Rating_SE" # fills the matrix with the standard errors
)
se_matrix = se_matrix.reindex(columns=runtime_order)

# Plot the mean Bayesian ratings as a heatmap
plt.figure(figsize=(8, 6))

plt.imshow( # takes the numerical matrix and represents each value as a colour
    rating_matrix,
    cmap="viridis", # colorblind-friendly colour scale
    aspect="auto" # adjusts the cells to fit the figure
)

# Display the standard error inside each heatmap cell
for row in range(len(se_matrix.index)): # goes through every genre row
    for col in range(len(se_matrix.columns)): # goes through every runtime column

        se = se_matrix.iloc[row, col] # gets the standard error for the current cell
        rating = rating_matrix.iloc[row, col] # gets the mean Bayesian rating for the current cell

        if pd.notna(se):

            # Use white text on darker cells and dark gray text on lighter cells
            if rating < 75:
                text_color = "white"
            else:
                text_color = "0.2"

            plt.text(
                col, row,
                f"±{se:.2f}", # displays the standard error with two decimal places
                ha="center",
                va="center",
                fontsize=7,
                color=text_color
            )

# Mark genre-runtime combinations with very low sample sizes
for row in range(len(entries_matrix.index)): # goes through every genre row
    for col in range(len(entries_matrix.columns)): # goes through every runtime column
        entries = entries_matrix.iloc[row, col] # gets the number of entries for the current cell
        if pd.notna(entries) and entries < 5: # checks if the group exists and contains fewer than 5 entries
            plt.text(
                col + 0.35, row - 0.0, "*", # places the asterisk in the upper-right corner of the cell
                ha="center",
                va="center",
                fontsize=12,
            )

# Add a label explaining the low-sample-size marker
plt.figtext(
    0.98, 0.01, # places the label near the lower-right corner
    "* Very low sample size (n < 5)",
    ha="right", 
    fontsize=8
)

# Add the genre names to the y-axis
plt.yticks(
    range(len(rating_matrix.index)), # creates one y-axis position for each genre
    rating_matrix.index,
    fontsize=9 # reduces the size of the genre labels
)

# Add runtime labels to the x-axis
runtime_labels = [
    "Short Films/\nMusic Videos",
    "Feature Films/\nShort Series",
    "Single-Cour",
    "Double-Cour",
    "Multi-Cour",
    "Long-Run"
]

plt.xticks(
    range(len(runtime_labels)),
    runtime_labels,
    fontsize=9 
)

plt.xlabel("Runtime Classification")
plt.ylabel("Genre")
plt.title("Mean Bayesian Rating and Uncertainty by Genre and Runtime")

# Add a colour scale showing what the colours represent
plt.colorbar(label="Mean Bayesian Rating")

plt.tight_layout()
plt.show()



# Figure 3: Runtime-Rating Correlation by Genre
# Analyze the correlation between total runtime and Bayesian rating for each genre.

# Store the genre names in alphabetical order
genres = sorted(anime_data["Genre"].unique()) # unique() finds all unique genres

# Store the Spearman correlation for each genre
genre_correlations = {}

# Calculate Spearman correlation separately for each genre
for genre in genres: 
    genre_data = anime_data[anime_data["Genre"] == genre] # selects only anime belonging to the current genre

    rho, p_value = spearmanr( # calculates the Spearman correlation coefficient and its associated p-value
        genre_data["Runtime"], # runtime values
        genre_data["Bayesian_Rating"] # corresponding Bayesian rating values
    )

    # Calculate the approximate standard error of the Spearman correlation
    n = len(genre_data) # number of anime entries used to calculate the correlation
    rho_se = np.sqrt((1 - rho**2) / (n - 2)) # estimates the uncertainty of the correlation coefficient

    genre_correlations[genre] = (rho, rho_se) # stores the correlation coefficient and its uncertainty for the current genre
    
# Create Figure 3 with 20 subplot positions for the 19 genres
plt.figure(figsize=(10, 6))

# Create one runtime-rating scatterplot for each genre
for i, genre in enumerate(genres):

    plt.subplot(4, 5, i + 1) # creates each genre plot in a 4-row by 5-column layout

    # Select only anime belonging to the current genre
    genre_data = anime_data[anime_data["Genre"] == genre]

    # Plot individual anime runtime against Bayesian rating
    plt.scatter(
        genre_data["Runtime"],
        genre_data["Bayesian_Rating"],
        color="tab:orange",
        alpha=0.25, # makes overlapping points easier to see
        s=2 # controls the size of each point
    )

    # Calculate a trend line using logarithmic runtime
    log_runtime = np.log10(genre_data["Runtime"])

    trend = np.polyfit(
        log_runtime,
        genre_data["Bayesian_Rating"],
        1 # fits a straight line to the data
    )

    # Create x-values for displaying the trend line
    trend_x = np.linspace(
        log_runtime.min(),
        log_runtime.max(),
        100 # creates 100 values so the trend line appears smooth
    )

    # Plot the trend line
    plt.plot(
        10 ** trend_x, # converts logarithmic runtime back to runtime values
        trend[0] * trend_x + trend[1], # calculates the rating predicted by the trend line
        color="black",
        linewidth=1
    )

    # Define the same positive runtime range for all genre plots
    runtime_min = anime_data["Runtime"].min() # finds the smallest runtime 
    runtime_max = anime_data["Runtime"].max() # finds the largest runtime

    # Define the same Bayesian rating limits for all genre plots
    rating_min = anime_data["Bayesian_Rating"].min() - 2
    rating_max = anime_data["Bayesian_Rating"].max() + 2 

    plt.xscale("log")
    plt.xlim(runtime_min, runtime_max) # uses the same observed runtime range for every genre
    plt.ylim(rating_min, rating_max) # uses the same observed rating range for every genre
    plt.title(genre)

    rho, rho_se = genre_correlations[genre] # gets the correlation coefficient and its uncertainty
    n = len(genre_data) # gets the numbermber of entries in the current genre 

    # Display Spearman correlation and number of entries
    plt.text(
        0.05, 0.40,
        f"ρ = {rho:.3f} ± {rho_se:.3f}\nn = {n}",
        transform=plt.gca().transAxes,
        va="top",
        fontsize=8
    )


# Add labels for the whole figure
plt.figtext(
    0.5, 0.01,
    "Total Runtime (minutes, log scale)",
    ha="center"
)

plt.figtext(
    0.01, 0.5,
    "Bayesian Rating",
    va="center",
    rotation="vertical"
)

plt.suptitle("Runtime-Rating Correlation by Genre")

plt.tight_layout(rect=[0.03, 0.03, 1, 0.96])
plt.show()
