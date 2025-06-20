#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#define MAX_ENTRIES 10000
#define MAX_TIMESTAMP_LEN 20
#define MAX_VALUE_LEN 10

typedef struct {
    char timestamp[MAX_TIMESTAMP_LEN];
    double value;
} TimeValue; //struct that stores each timestamp and temperature/humidity

typedef struct {
    TimeValue hum_entries[MAX_ENTRIES];
    TimeValue temp_entries[MAX_ENTRIES];
    int count;
} DataSet;

char* readTextFile(const char *filename); //reads the txt file and stores its data into a string and returns it
void trimTempData(const char *buffer, DataSet *dataset); //trims and filters the data stored in the buffer, parses the timestamps and temperatures and stores them into a DataSet struct
void trimHumData(const char *buffer, DataSet *dataset); //trims and filters the data stored in the buffer, parses the timestamps and humidity and stores them into a DataSet struct
void swapTimeValue(TimeValue *a, TimeValue *b); //swap function but for TimeValue values
void quicksort_by_timestamps(TimeValue *array, int left, int right);
int compare_timestamps(const char* ts1, const char* ts2);
int interpolate_position(TimeValue *array, int left, int right, const char* target); // find position with interpolation for TimeValue arrays
int linear_search(TimeValue *array, int left, int right, const char* target); // linear search in TimeValue arrays
int binary_interpolation_search(TimeValue *array, int n, const char* target); //bis in a TimeValue array, returns -1 if not found
int search_temperature_by_timestamp(DataSet *dataset, const char* target_timestamp); //uses bis_star() to search for the temperature
int search_humidity_by_timestamp(DataSet *dataset, const char* target_timestamp); //uses bis_star() to search for humidity
void search_value(DataSet *dataset, const char* timestamp, int option); //called in the main function and uses the 2 functions above to present the asked values to the user
int bis_star(TimeValue *array, int n, const char* target); //an edition of the binary_interpolation_search() function to improve the worst case, returns -1 if not found

int main() {

    DataSet aarhus_data;
    char timestamp;
    int option=0;
    char* filename1 = "tempm.txt";
    char* filename2 = "hum.txt";
    char* buffer_temp = readTextFile(filename1);
    if (buffer_temp == NULL) {
        printf("Error reading file\n");
        return 1;
    }
    char* buffer_hum = readTextFile(filename2);
    if (buffer_hum == NULL) {
        printf("Error reading file\n");
        return 1;
    }
    trimTempData(buffer_temp, &aarhus_data);
    trimHumData(buffer_hum, &aarhus_data);
    free(buffer_hum);
    free(buffer_temp);
    quicksort_by_timestamps(aarhus_data.temp_entries, 0, aarhus_data.count - 1);
    quicksort_by_timestamps(aarhus_data.hum_entries, 0, aarhus_data.count - 1);

    printf("Which day do you want data from?:");
    scanf("%s", &timestamp);
    if (bis_star(aarhus_data.temp_entries, aarhus_data.count, &timestamp) == -1 || bis_star(aarhus_data.hum_entries, aarhus_data.count, &timestamp) == -1) {
        printf("Incorrect timestamp.\n");
        return 0;
    }
    printf("\nType 1 for temperature data, 2 for humidity data, or 3 for both:");
    scanf("%d", &option);
    printf("\n");
    search_value(&aarhus_data, &timestamp, option);



    return 0;
}

char* readTextFile(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error reading file\n");
        return NULL;
    }

    fseek(file, 0, SEEK_END); //move pointer to eof
    long size = ftell(file); //get the size of the file in bytes
    rewind(file); //get the pointer back to the beginning of the file

    char *buffer = (char *)malloc(size + 1); // +1 is for the '\0' character
    if (buffer == NULL) {
        printf("Error allocating memory\n");
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, size, file); //read the file into the buffer
    buffer[size] = '\0'; //add the '\0' character to the end of the buffer

    fclose(file);

    return buffer;
}

void trimTempData(const char *buffer, DataSet *dataset) {
    const char *p = buffer;
    dataset->count = 0;

    while (*p && dataset->count < MAX_ENTRIES) {
        // Look for pattern: "2014-"
        const char *timestamp_start = strstr(p, "2014-");
        if (!timestamp_start) break;

        // Back up to find the opening quote
        const char *quote_start = timestamp_start - 1;
        while (quote_start > p && *quote_start != '"' && *quote_start != '\'' &&
               *quote_start != '"' && *quote_start != '"') {
            quote_start--;
        }

        if (*quote_start == '"' || *quote_start == '\'' || *quote_start == '"' || *quote_start == '"') {
            // Find the end of timestamp
            const char *timestamp_end = timestamp_start;
            while (*timestamp_end && *timestamp_end != '"' && *timestamp_end != '\'' &&
                   *timestamp_end != '"' && *timestamp_end != '"') {
                timestamp_end++;
            }

            // Copy timestamp
            size_t timestamp_length = timestamp_end - timestamp_start;
            if (timestamp_length < MAX_TIMESTAMP_LEN) {
                strncpy(dataset->temp_entries[dataset->count].timestamp, timestamp_start, timestamp_length);
                dataset->temp_entries[dataset->count].timestamp[timestamp_length] = '\0';

                // Find the colon and value
                const char *colon_pos = strchr(timestamp_end, ':'); //returns the location of the first occurrence of the ':' character
                if (colon_pos) {
                    const char *value_start = colon_pos + 1;

                    // Skip whitespace and quotes
                    while (*value_start && (*value_start <= ' ' || *value_start == '"' ||
                           *value_start == '\'' || *value_start == '"' || *value_start == '"')) {
                        value_start++;
                    }


                    if (*value_start && (isdigit(*value_start) || *value_start == '-' ||
                        *value_start == '+' || *value_start == '.')) {
                        char *value_end;
                        dataset->temp_entries[dataset->count].value = strtod(value_start, &value_end); // convert to double

                        if (value_end > value_start) {
                            dataset->count++;
                        }
                    }
                }
            }
        }

        // Move past this timestamp for next iteration
        p = timestamp_start + 1;
    }
}

void trimHumData(const char *buffer, DataSet *dataset) {
    const char *p = buffer;
    dataset->count = 0; //initializing the count again because we know that we have the same number of entries for both temperatures and humidity

    while (*p && dataset->count < MAX_ENTRIES) {
        // Look for pattern: "2014-"
        const char *timestamp_start = strstr(p, "2014-");
        if (!timestamp_start) break;

        // Back up to find the opening quote
        const char *quote_start = timestamp_start - 1;
        while (quote_start > p && *quote_start != '"' && *quote_start != '\'' &&
               *quote_start != '"' && *quote_start != '"') {
            quote_start--;
        }

        if (*quote_start == '"' || *quote_start == '\'' || *quote_start == '"' || *quote_start == '"') {
            // Find the end of timestamp
            const char *timestamp_end = timestamp_start;
            while (*timestamp_end && *timestamp_end != '"' && *timestamp_end != '\'' &&
                   *timestamp_end != '"' && *timestamp_end != '"') {
                timestamp_end++;
            }

            // Copy timestamp
            size_t timestamp_length = timestamp_end - timestamp_start;
            if (timestamp_length < MAX_TIMESTAMP_LEN) {
                strncpy(dataset->hum_entries[dataset->count].timestamp, timestamp_start, timestamp_length);
                dataset->hum_entries[dataset->count].timestamp[timestamp_length] = '\0';

                // Find the colon and value
                const char *colon_pos = strchr(timestamp_end, ':'); //returns the location of the first occurrence of the ':' character
                if (colon_pos) {
                    const char *value_start = colon_pos + 1;

                    // Skip whitespace and quotes
                    while (*value_start && (*value_start <= ' ' || *value_start == '"' ||
                           *value_start == '\'' || *value_start == '"' || *value_start == '"')) {
                        value_start++;
                    }


                    if (*value_start && (isdigit(*value_start) || *value_start == '-' ||
                        *value_start == '+' || *value_start == '.')) {
                        char *value_end;
                        dataset->hum_entries[dataset->count].value = (int)strtol(value_start, &value_end, 10); // convert to int (strtol converts to long int so we typecast)

                        if (value_end > value_start) {
                            dataset->count++;
                        }
                    }
                }
            }
        }

        // Move past this timestamp for next iteration
        p = timestamp_start + 1;
    }
}

void swapTimeValue(TimeValue *a, TimeValue *b) {
    TimeValue temp = *a;
    *a = *b;
    *b = temp;
}

void quicksort_by_timestamps(TimeValue *array, int left, int right) {
    int leftArrow, rightArrow;
    char pivot[MAX_TIMESTAMP_LEN];

    if (left >= right) return;  // Base case

    leftArrow = left;
    rightArrow = right;

    // Choose pivot timestamp (middle element)
    strcpy(pivot, array[(left + right) / 2].timestamp);

    while (leftArrow <= rightArrow) {
        // Find element on left that should be on right
        while (strcmp(array[leftArrow].timestamp, pivot) < 0) leftArrow++;

        // Find element on right that should be on left
        while (strcmp(array[rightArrow].timestamp, pivot) > 0) rightArrow--;

        // Swap if we found a pair
        if (leftArrow <= rightArrow) {
            swapTimeValue(&array[leftArrow], &array[rightArrow]);
            leftArrow++;
            rightArrow--;
        }
    }

    // Recursively sort the partitions
    if (left < rightArrow) quicksort_by_timestamps(array, left, rightArrow);
    if (right > leftArrow) quicksort_by_timestamps(array, leftArrow, right);
}

int compare_timestamps(const char* ts1, const char* ts2) {
    return strcmp(ts1, ts2);
}

int interpolate_position(TimeValue *array, int left, int right, const char* target) {

    int cmp_left = compare_timestamps(target, array[left].timestamp);
    int cmp_right = compare_timestamps(target, array[right].timestamp);

    // if the target is out of boundaries
    if (cmp_left <= 0) return left;
    if (cmp_right >= 0) return right;

    double ratio = 0.5; // Default position


    if (strlen(target) >= 10 && strlen(array[left].timestamp) >= 10 && strlen(array[right].timestamp) >= 10) {
        // comparison of first 10 characters (YYYY-MM-DD)
        char target_date[11], left_date[11], right_date[11];
        strncpy(target_date, target, 10); target_date[10] = '\0';
        strncpy(left_date, array[left].timestamp, 10); left_date[10] = '\0';
        strncpy(right_date, array[right].timestamp, 10); right_date[10] = '\0';

        if (strcmp(target_date, left_date) > 0 && strcmp(right_date, left_date) > 0) {

            int left_year = atoi(left_date);
            int right_year = atoi(right_date);
            int target_year = atoi(target_date);

            if (right_year != left_year) {
                ratio = (double)(target_year - left_year) / (right_year - left_year);
                ratio = (ratio < 0) ? 0 : (ratio > 1) ? 1 : ratio;
            }
        }
    }

    int pos = left + (int)(ratio * (right - left));
    return (pos < left) ? left : (pos > right) ? right : pos;
}

int linear_search(TimeValue *array, int left, int right, const char* target) {
    for (int i = left; i <= right; i++) {
        if (compare_timestamps(array[i].timestamp, target) == 0) {
            return i;  // successful
        }
    }
    return -1;
}

int binary_interpolation_search(TimeValue *array, int n, const char* target) {
    if (n <= 0) return -1;

    int left = 0;
    int right = n - 1;
    int size = right - left + 1;
    int next;


    next = interpolate_position(array, left, right, target);

    while (compare_timestamps(target, array[next].timestamp) != 0) {
        int i = 0;
        size = right - left + 1;

        // using linear search if the size is small
        if (size < 3) {
            return linear_search(array, left, right, target);
        }

        int sqrt_size = (int)sqrt(size);
        if (sqrt_size < 1) sqrt_size = 1;

        if (compare_timestamps(target, array[next].timestamp) > 0) {
            // search to the right
            i = 1;
            while (next + i * sqrt_size - 1 <= right &&
                   compare_timestamps(target, array[next + i * sqrt_size - 1].timestamp) > 0) {
                i++;
            }
            right = (next + i * sqrt_size <= right) ? next + i * sqrt_size : right;
            left = next + (i - 1) * sqrt_size;
        }
        else if (compare_timestamps(target, array[next].timestamp) < 0) {
            // search to the left
            i = 1;
            while (next - i * sqrt_size + 1 >= left &&
                   compare_timestamps(target, array[next - i * sqrt_size + 1].timestamp) < 0) {
                i++;
            }
            right = next - (i - 1) * sqrt_size;
            left = (next - i * sqrt_size >= left) ? next - i * sqrt_size : left;
        }

        if (left <= right) {
            next = interpolate_position(array, left, right, target);
        } else {
            return -1;  // fail
        }

        // to avoid infinite loop
        if (left > right) {
            return -1;  // fail
        }
    }

    // sucesss
    return next;
}

int search_temperature_by_timestamp(DataSet *dataset, const char* target_timestamp) {
    return bis_star(dataset->temp_entries, dataset->count, target_timestamp);
}


int search_humidity_by_timestamp(DataSet *dataset, const char* target_timestamp) {
    return bis_star(dataset->hum_entries, dataset->count, target_timestamp);
}

void search_value(DataSet *dataset, const char* timestamp, int option) {
        switch (option) {
            case 1:
                printf("Temperature: %.2f\n", dataset->temp_entries[search_temperature_by_timestamp(dataset, timestamp)].value);
                break;
            case 2:
                printf("Humidity: %d\n", (int)dataset->hum_entries[search_humidity_by_timestamp(dataset, timestamp)].value);
                break;
            case 3:
                printf("Temperature: %.2f\n", dataset->temp_entries[search_temperature_by_timestamp(dataset, timestamp)].value);
                printf("Humidity: %d\n", (int)dataset->hum_entries[search_humidity_by_timestamp(dataset, timestamp)].value);
                break;
            default:
                printf("Invalid option\n");
                break;
        }
}

int bis_star(TimeValue *array, int n, const char* target) {
    if (n <= 0) return -1;

    int left = 0;
    int right = n - 1;
    int size = right - left + 1;
    int next;
    int iterations = 0;
    int max_iterations = (int)(log2(n)) + 3; // Allow some extra iterations beyond binary search
    int last_size = size;
    int progress_stall_count = 0;

    next = interpolate_position(array, left, right, target);

    while (compare_timestamps(target, array[next].timestamp) != 0) {
        iterations++;
        size = right - left + 1;

        // Check for lack of progress - if size isn't reducing well, switch to binary search
        if (size >= last_size * 0.8) {
            progress_stall_count++;
        } else {
            progress_stall_count = 0;
        }
        last_size = size;

        // Fallback conditions:
        // 1. Too many iterations (worse than binary search)
        // 2. Size is small enough for linear search
        // 3. Progress has stalled (poor interpolation)
        if (iterations > max_iterations || size < 3 || progress_stall_count >= 2) {
            // Switch to binary search for guaranteed O(log n) behavior
            while (left <= right) {
                int mid = left + (right - left) / 2;
                int cmp = compare_timestamps(target, array[mid].timestamp);

                if (cmp == 0) {
                    return mid;  // Found
                } else if (cmp > 0) {
                    left = mid + 1;
                } else {
                    right = mid - 1;
                }
            }
            return -1;  // Not found
        }

        int sqrt_size = (int)sqrt(size);
        if (sqrt_size < 1) sqrt_size = 1;

        if (compare_timestamps(target, array[next].timestamp) > 0) {
            // Search to the right
            int i = 1;
            while (next + i * sqrt_size - 1 <= right &&
                   compare_timestamps(target, array[next + i * sqrt_size - 1].timestamp) > 0) {
                i++;
            }
            right = (next + i * sqrt_size <= right) ? next + i * sqrt_size : right;
            left = next + (i - 1) * sqrt_size;
        }
        else if (compare_timestamps(target, array[next].timestamp) < 0) {
            // Search to the left
            int i = 1;
            while (next - i * sqrt_size + 1 >= left &&
                   compare_timestamps(target, array[next - i * sqrt_size + 1].timestamp) < 0) {
                i++;
            }
            right = next - (i - 1) * sqrt_size;
            left = (next - i * sqrt_size >= left) ? next - i * sqrt_size : left;
        }

        if (left <= right) {
            next = interpolate_position(array, left, right, target);
        } else {
            return -1;  // Not found
        }

        // to avoid infinite loop
        if (left > right) {
            return -1;  // Not found
        }
    }

    // Success
    return next;
}

