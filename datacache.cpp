/********************************************************
*							*
*	Name: Chris LoGuidice				*
*	Class: CDA3101					*
*	Assignment: Implementing a Data Cache Simulator	*
*	Compile: "g++ datacache.cpp"			*
*							*
*********************************************************/	

#include<iostream>
#include<string>
#include<cstdlib>
#include<fstream>
#include<cctype>
#include<cmath>
#include<iomanip>
#include<stdio.h>


using namespace std;

// const ints
const int MAX_SETS = 8192;
const int MAX_SIZE = 8;		// MAX SET SIZE (ASSOCIATIVITY)
// Struct

// struct for possible memory in the cache
struct Cache {
	char acc_type;			// 'R' or 'W'
	int var_size;			// 1, 2, 4, or 8 bytes
	unsigned int hex_add;		// store hex value of address
	unsigned int tag;		// stores tag value
	unsigned int index;		// stores index value
	unsigned int offset;		// stores offset value
	string result;			// "hit" or "miss"
	int memref;			// 0, 1, or 2
};

// struct for associativity table
struct Table {
	bool valid[MAX_SIZE];			// column is valid or not
	int lru;				// least recently used in a column
	unsigned int tag_stored[MAX_SIZE];	// stores tag value	
	int dirty_bit[MAX_SIZE];		// for write
};


// prototypes
void Open_Config(int&, int&, int&);
void Calc_Phys_Add(Cache&, int, int, unsigned int);
void Scan_Line(Cache[], Table[], int&, int, int, int, int, const int);
void Simulation(Cache[], Table[], int, int, int);
void Print_Results(Cache[], int, int&, int&, const int);
void Print_Summary(Cache[], int, int, int);


int main()
{
	Cache Mem[MAX_SETS];		//stores each line for results output
	
	int num_lines = 0;
	int num_sets = 0;
	int set_size = 0;
	int byte_size = 0;
	int index_size;
	int offset_size;
	int tag_size;
	int hits = 0;
	int misses = 0;

	// open trace.config file to determine number of sets, set size and line size
	Open_Config(num_sets, set_size, byte_size);

	Table Assoc[num_sets];		// Associativty table

	cout << "Cache Configuration" << endl << endl
	     << "   " << num_sets << " " << set_size << "-way set associative entries"
	     << endl << "   of line size "<< byte_size << " bytes" << endl << endl 
	     << endl;
	int i;
	int z = byte_size;
	
	// calculate power of two for line size for number of bits for offset
	for(i = 0; z != 0; i++)
	{
		z = z / 2;
	}

	offset_size = i - 1;
	
	// for calculating the power of 2 for num of sets for number of bits for index
	int x = num_sets; 

	for(i = 0; x != 0; i++)
	{
		x = x / 2;
	}
	
	if(i == 1)	
		index_size = 1;
	else
		index_size = i - 1;

	// determine number of bits for the tag
	tag_size = 32 - offset_size - index_size;

	// initialize associativity table to 0
	for(i = 0; i < num_sets; i++)
	{
		for(int itr = 0; itr < set_size; itr++)
		{
			Assoc[i].valid[itr] = false;
			Assoc[i].lru = 0;
			Assoc[i].tag_stored[itr] = 0;
			Assoc[i].dirty_bit[itr] = 0;
		}
	}

	// sets up mem and assoc
	Scan_Line(Mem, Assoc, num_lines, index_size, offset_size, set_size, byte_size, MAX_SETS);
	
	// print result table
	Print_Results(Mem, num_lines, hits, misses, MAX_SETS);
	
	// print summary table
	Print_Summary(Mem, num_lines, hits, misses);


	return 0;
}

//Called by: main
// opens trace.config and stores the 3 numbers needed for configuration
void Open_Config(int &num_sets, int &set_size, int &byte_size)
{
	ifstream file;
	string ch;
	int nums[3] = {0};

	file.open("trace.config");

	if(!file.is_open())
	{
		cout << "ERROR FILE COULD NOT BE OPENED" << endl;
		exit(-1);
	}
	
	int x = 0;
	int i = 0;
	char temp[6];
	while(getline(file, ch)) 
	{
		i = 0;
		while(!(isdigit(ch[i])))
		{
			i++;
		}
		
		int z = 0;
		while(isdigit(ch[i]))
		{
			temp[z] = ch[i];
			i++;
			z++;
			temp[z] = '\0';
		}	
	
		nums[x] = atoi(temp);
		x++;
	}
	
	file.close();

	num_sets = nums[0];
	set_size = nums[1];
	byte_size = nums[2];
		
}

//Called by: main
//reads the trace.dat file to get each index of the cache struct
//calls calc_phys_add in order to separate bits for offset, tag and index
//then calls simulation which moves a line from the cache struct to the assoc table
//if the data is a miss to move up in data hierachy
void Scan_Line(Cache Mem[], Table Assoc[], int &num_lines, int index_size, 
			int offset_size, int set_size, int t_size, const int MAX_SETS)
{
	char *line;
	int acc;
	int var;
	int hex;
	for(int i = 0; i < MAX_SETS; i++)
        {	
		string r;
		line = new char[MAX_SETS];

                if(cin)
		{
                        getline(cin, r);
			num_lines++;
		}
                else
			break;
                
		int x = 0;
                
                while(r[x] != '\0')
		{
                        line[x] = r[x];
                        x++;
			line[x] = '\0';
                }

		sscanf(line, "%c:%d:%x", &acc, &var, &hex);
		
		Mem[i].acc_type = acc;
		Mem[i].var_size = var;
		Mem[i].hex_add = (int32_t) hex;

		// splits the hex address
		Calc_Phys_Add(Mem[i], index_size, offset_size, hex);
	
		//determines hit or miss (Associativity Table)
		Simulation(Mem, Assoc, i, set_size, t_size);
	}
}

//called by: Scan_Line
//uses variable mask to obtain if a is set or not if it is, 1 is set on offset
//and then shifts the bits
//determines offset, then index, and the remaining bits are the tag
void Calc_Phys_Add(Cache &add, int index_size, int offset_size, unsigned int hex)
{
	int mask;
	unsigned int tar = 0;
	int y = 1;
	
	//offset
	for(int i = 0; i < offset_size; i++)
	{
		mask = hex & 1;

		hex = hex >> 1;

		if(mask == 1)
		{
			tar |= 1 << i;
		}
	}
	add.offset = tar;
	
	//reset for index
	tar = 0;
	
	//index
	for(int i = 0; i < index_size; i++)
	{
		mask = hex & 1;
	
		hex = hex >> 1;

		if(mask == 1)
		{
			tar |= 1 << i;
		}
	}
	add.index = tar;
	
	//tag is remaining bits
	add.tag = hex;
}


//Called by: Scan_Line
//Searches each index of Assoc based on Mem's index (gives the row that is being examined)
//The for loop searches each column based on the set_size (number of sets) 
void Simulation(Cache Mem[], Table Assoc[], int pos, int set_size, int t_size)
{
	int i;
	for(i = 0; i < set_size; i++)
	{
		// Exists in the table
		if((Assoc[Mem[pos].index].valid[i] == true) && 
			(Assoc[Mem[pos].index].tag_stored[i] == Mem[pos].tag))
		{
			Mem[pos].result = "hit";
			Mem[pos].memref = 0;
			
			if(Assoc[Mem[pos].index].lru == i)
				Assoc[Mem[pos].index].lru++;
			
			if(Assoc[Mem[pos].index].lru == set_size)
			{
				Assoc[Mem[pos].index].lru = 0;
			}

			if(Mem[pos].acc_type == 'W')
			{
				Assoc[Mem[pos].index].tag_stored[i] == Mem[pos].tag;
				Assoc[Mem[pos].index].dirty_bit[i] = 1;
			}
			
			return;
		}
	}
	
	Mem[pos].result = "miss";
	Mem[pos].memref = 1;

	// Adds a tag to the table if a miss at the specified index
	//case for original validation of an index
	for(int j = 0; j < set_size; j++)
	{
		if((Assoc[Mem[pos].index].valid[j] == false) && 
			(Assoc[Mem[pos].index].tag_stored[j] == 0))
		{
			Assoc[Mem[pos].index].valid[j] = true;
			
			if(Assoc[Mem[pos].index].lru == j)
				Assoc[Mem[pos].index].lru++;

			if(Assoc[Mem[pos].index].lru == set_size)
				Assoc[Mem[pos].index].lru = 0;

			Assoc[Mem[pos].index].tag_stored[j] = Mem[pos].tag;	
			

			if(Mem[pos].acc_type == 'W')
                        {
                                Assoc[Mem[pos].index].dirty_bit[j] = 1;
                        }
		
			return;
		}
	}
	
	// If the tag is valid  and set size is 1
	if(set_size == 1)
	{
		Assoc[Mem[pos].index].valid[0] = true;
        	Assoc[Mem[pos].index].lru = 0;
        	Assoc[Mem[pos].index].tag_stored[0] = Mem[pos].tag;

	
		if((Mem[pos].acc_type == 'R') && (Assoc[Mem[pos].index].dirty_bit[0] == 1))
                {
                        Mem[pos].memref = 2;
			Assoc[Mem[pos].index].dirty_bit[0] = 0;
                }

		if((Mem[pos].acc_type == 'W') && (Assoc[Mem[pos].index].dirty_bit[0] == 1))
                {
			Mem[pos].memref = 2;
                }


		if(Mem[pos].acc_type == 'W')
                {
                        Assoc[Mem[pos].index].dirty_bit[0] = 1;
                }

		return;
	}
	

	//search for if the set size is > 1
	//find next open spot after activation
	//if all have been activated then it is set to the lru bit
	int j;
	for(j = 0; j < set_size; j++)
	{
		if(Assoc[Mem[pos].index].valid[j] == false)
		{	
			break;
		}

		if(j == set_size-1)
		{
			j = Assoc[Mem[pos].index].lru;
			break;
		}
	}

	// Assigns value to the table
	// j == set_size - 1 then restart back at 0
	if(j < set_size)
	{
		Assoc[Mem[pos].index].valid[j] = true;
		Assoc[Mem[pos].index].lru = j+1;
		Assoc[Mem[pos].index].tag_stored[j] = Mem[pos].tag;
		
		 if(Assoc[Mem[pos].index].lru == set_size)
                 {
                        Assoc[Mem[pos].index].lru = 0;
                 }

		if((Mem[pos].acc_type == 'R') && (Assoc[Mem[pos].index].dirty_bit[j] == 1))
                {
			Mem[pos].memref = 2;
                        Assoc[Mem[pos].index].dirty_bit[j] = 0;

                }

		if((Mem[pos].acc_type == 'W') && (Assoc[Mem[pos].index].dirty_bit[j] == 1))
                {
                        Mem[pos].memref = 2;
                }

		if(Mem[pos].acc_type == 'W')
                {
                        Assoc[Mem[pos].index].dirty_bit[j] = 1;
                }

	}
	else
	{
		Assoc[Mem[pos].index].valid[0] = true;
		Assoc[Mem[pos].index].tag_stored[0] = Mem[pos].tag;
		Assoc[Mem[pos].index].lru = 0;
	
		if((Mem[pos].acc_type == 'R') && (Assoc[Mem[pos].index].dirty_bit[0] == 1))
                {
			Mem[pos].memref = 2;
			Assoc[Mem[pos].index].dirty_bit[0] = 0;
                }

		if((Mem[pos].acc_type == 'W') && (Assoc[Mem[pos].index].dirty_bit[0] == 1))
                {
                        Mem[pos].memref = 2;
                }


		if(Mem[pos].acc_type == 'W')
                {
                        Assoc[Mem[pos].index].dirty_bit[0] = 1;
                }


	}
}


//Called by: main
//Prints out the content contained in Mem into a table
//Records all hits and misses read also
// Error checks for illegal size and misaligned references and those do not affect
//the miss and hit counts
void Print_Results(Cache Mem[], int num_lines, int &hits, int &misses, const int MAX_SETS)
{
	cout << "Results for Each Reference" << endl << endl;

	cout << "Ref  Access Address    Tag   Index Offset Result Memrefs" << endl
	     << "---- ------ -------- ------- ----- ------ ------ -------"  << endl;
	
	for(int i = 0; i < num_lines - 1; i++)
	{
		// Check illegal size
		if((Mem[i].var_size != 1) && (Mem[i].var_size != 2) && (Mem[i].var_size != 4) && (Mem[i].var_size != 8))
                {
                        cerr << "line " << i+1 << " has illegal size " << Mem[i].var_size << endl;
                        continue;
                }

		//checks misaligned reference
		int a = Mem[i].offset;
	
		a = a % Mem[i].var_size;

		if(a != 0)
		{
			cerr << "line " << i+1 << " has misaligned reference at address ";
			printf("%x", Mem[i].hex_add);
			cerr << " for size " << Mem[i].var_size << endl;
			continue;
		}

	
		// reference #
		cout << setw(4) << i+1;
		

		if(Mem[i].acc_type == 'R')
			cout << setw(7) << "read";
		else if(Mem[i].acc_type == 'W')
			cout << setw(7) << "write";
	

		if(Mem[i].result == "hit")
			hits++;
		else if(Mem[i].result == "miss")
			misses++;


		printf("%9x", Mem[i].hex_add);
	
		
		printf("%8x", Mem[i].tag);
	

		cout << setw(6) << Mem[i].index;
	
		cout << setw(7) << Mem[i].offset;
		
		cout << setw(7) << Mem[i].result;
	
		cout << setw(8) << Mem[i].memref;

		cout << endl;
	}
	cout << endl;
}


//Called by: main
// prints out the final results for the hits and misses from the simulation
// also prints the total, hit ratio and miss ratio
void Print_Summary(Cache Mem[], int num_lines, int hits, int misses)
{
	float hit_ratio;
	float miss_ratio;

	int total;

	total = hits + misses;

	cout << endl << "Simulation Summary Statistics" << endl
	     << "-----------------------------" << endl;

	hit_ratio = static_cast<float>(hits) / static_cast<float>(total);

	miss_ratio = 1 - hit_ratio;

	cout << "Total hits       : " << hits << endl
	     << "Total misses     : " << misses << endl
	     << "Total accesses   : " << total << endl
	     << "Hit ratio        : " << fixed << setprecision(6) << hit_ratio << endl
	     << "Miss ratio       : " << fixed << setprecision(6) << miss_ratio << endl << endl;

}
