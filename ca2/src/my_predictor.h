class my_update : public branch_update
{
public:
	unsigned int index;
};

class my_predictor : public branch_predictor
{
public:
#define GLOBAL_HISTORY_LENGTH 30 // Global history length, stores up to 30 branch outcomes (1 = taken, 0 = not taken)
#define TABLE_BITS 30			 // Number of entries in the prediction table
#define SHORT_HISTORY_LENGTH 2	 // Shorter history length for branches needing quick, recent information
#define MEDIUM_HISTORY_LENGTH 8	 // Medium history length for branches that may benefit from intermediate history
#define WEIGHT 0.8				 // Weight of the current branch outcome in history selection
	my_update u;
	branch_info bi;
	unsigned int global_history, short_history, medium_history;
	unsigned char prediction_table[1 << TABLE_BITS];   // Table storing branch predictions
	unsigned int prediction_accuracy[1 << TABLE_BITS]; // Tracks the accuracy of local vs. combined histories
	unsigned int index;								   // Index of the last accessed branch in prediction_accuracy
	bool global_vs_local[2];						   // Stores global vs. local prediction outcomes
	bool short_vs_medium_long[3];					   // Stores which history (short, medium, long) performed best
	unsigned int history_preference[1 << TABLE_BITS];  // Tracks which history length (short, medium, long) is preferred
	unsigned int previous_outcome[1 << TABLE_BITS];	   // Stores previous outcomes for history length preference

	my_predictor(void) : global_history(0)
	{
		memset(prediction_table, 0, sizeof(prediction_table));
	}

	branch_update *predict(branch_info &b)
	{
		bi = b;
		int global_index, short_index, medium_index, local_index;
		unsigned int selected_history;

		if (b.br_flags & BR_CONDITIONAL)
		{
			global_index = global_history << (TABLE_BITS - GLOBAL_HISTORY_LENGTH);
			short_index = short_history << (TABLE_BITS - SHORT_HISTORY_LENGTH);
			medium_index = medium_history << (TABLE_BITS - MEDIUM_HISTORY_LENGTH);
			local_index = b.address & ((1 << TABLE_BITS) - 1);

			// Update short_vs_medium_long based on which history gives the best prediction
			short_vs_medium_long[0] = prediction_table[global_index ^ local_index] >> 2; // Long history prediction
			short_vs_medium_long[1] = prediction_table[medium_index ^ local_index] >> 2; // Medium history prediction
			short_vs_medium_long[2] = prediction_table[short_index ^ local_index] >> 2;	 // Short history prediction

			index = global_index ^ local_index;

			int weight = (WEIGHT) * (history_preference[index]) + (1 - WEIGHT) * (previous_outcome[index]);

			// Determine history length to use based on the weighted result
			if (weight >= 1 && weight < 2 * WEIGHT + (1 - WEIGHT))
			{
				global_index = medium_index;
			}
			else if (weight >= 2 * WEIGHT + (1 - WEIGHT))
			{
				global_index = short_index;
			}

			// Select prediction type based on counter accuracy
			if (prediction_accuracy[index] == 0)
			{
				// Only use local history if it has proven more accurate
				u.index = local_index;
			}
			else
			{
				// Use combination of global and local history if more accurate
				u.index = global_index ^ local_index;
			}

			// Update global_vs_local with the outcome from global and combined history predictions
			global_vs_local[0] = prediction_table[global_index] >> 2;				// Global history outcome
			global_vs_local[1] = prediction_table[global_index ^ local_index] >> 2; // Combined outcome

			// Shift by 2 to adjust for larger taken/not taken size
			u.direction_prediction(prediction_table[u.index] >> 2);
		}
		else
		{
			// For non-conditional branches, always predict taken
			u.direction_prediction(true);
		}

		// Not relevant in conditional predictions
		u.target_prediction(0);
		return &u;
	}

	void update(branch_update *u, bool taken, unsigned int target)
	{
		// Only update if branch is conditional
		if (bi.br_flags & BR_CONDITIONAL)
		{
			unsigned char *counter = &prediction_table[((my_update *)u)->index];

			if (taken && *counter < 10)
			{
				(*counter)++;
			}
			else if (!taken && *counter > 0)
			{
				(*counter)--;
			}

			// Update global history with outcome of this branch
			global_history <<= 1;
			global_history |= taken;
			global_history &= (1 << GLOBAL_HISTORY_LENGTH) - 1;

			// Update short history with outcome of this branch
			short_history <<= 1;
			short_history |= taken;
			short_history &= (1 << SHORT_HISTORY_LENGTH) - 1;

			// Update medium history with outcome of this branch
			medium_history <<= 1;
			medium_history |= taken;
			medium_history &= (1 << MEDIUM_HISTORY_LENGTH) - 1;

			// Track accuracy
			// Update prediction accuracy based on local vs. combined history accuracy
			if (global_vs_local[1] == taken)
			{
				// Combined history more accurate
				prediction_accuracy[index] = 1;
			}
			else if (global_vs_local[0] == taken)
			{
				// Local history more accurate
				prediction_accuracy[index] = 0;
			}

			// Record previous history length used
			previous_outcome[index] = history_preference[index];

			// Update history length preference based on outcome accuracy
			if (short_vs_medium_long[0] == taken)
			{
				// Long history is preferred
				history_preference[index] = 0;
			}
			else if (short_vs_medium_long[2] == taken)
			{
				// Short history is preferred
				history_preference[index] = 2;
			}
			else if (short_vs_medium_long[1] == taken)
			{
				// Medium history is preferred
				history_preference[index] = 1;
			}
		}
	}
};