// Name:		Amy Mason
// Course:		ECE 1305 - 002, Spring 2016
// Assignment:	Project 1
// Work Group:	None

#pragma comment(linker, "/STACK:16777216")
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cmath>
using namespace std;

bool get_file(string &infname, string &outfname, bool &grayscale);
string get_basename(string fname);
void read_file(string &infname, string &type, int &ncols, int &nrows, float &minElev, float &maxElev,
	float &xScale, float &yScale, float pix[]);
void illuminate(int ncols, int nrows, float illum[], float pix[], float sx, float sy, float sz,
	float xScale, float yScale);
void colorize(int ncols, int nrows, float minElev, float maxElev, float pix[], float illum[], float red[], float green[], float blue[]);
void write_file_pgm(string outfname, int ncols, int nrows, float illum[]);
void write_file_ppm(string outfname, int ncols, int nrows, float minElev, float maxElev, float illum[], float pix[]);

void norm(float x, float y, float z, float &xnorm, float &ynorm, float &znorm);
void surf_norm(float h0, float hx, float hy, float xScale, float yScale, float &a, float &b, float &c);
float scale(float pixel, float min, float max);
void ae2xyz(float az, float el, float &x, float &y, float &z);
float mag(float x, float y, float z);
float dotp(float x1, float y1, float z1, float x2, float y2, float z2);
void crossp(float x1, float y1, float z1, float x2, float y2, float z2, float &cx, float &cy, float &cz);

const int SIZE = 512 * 512;

int main()
{
	// Declare variables
	string infname, outfname, type;
	bool grayscale; 
	int ncols, nrows;
	float minElev, maxElev, xScale, yScale;
	float pix[SIZE] = { 0 };

	// calls function to open file and get output file name
	get_file(infname, outfname, grayscale);

	// Exits program if file didn't open
	if (get_file == false)
	{
		return 1;
	}

	// calls function to read file into program
	read_file(infname, type, ncols, nrows, minElev, maxElev, xScale, yScale, pix);

	// outputs header information
	cout << "The header information for " << infname << " is:" << endl
		<< type << " " << ncols << " " << nrows << " " << minElev << " " << maxElev << " "
		<< xScale << " " << yScale << endl << endl;

	float az, el, sx, sy, sz;

	// prompts user to input sun's positiion
	cout << "Please input the azimuth and elevation of the sun's position in radians:" << endl;
	cin >> az >> el;

	// calls function to convert azimuth and elevation to x y z
	ae2xyz(az, el, sx, sy, sz);

	float illum[SIZE] = { 0 };

	// calls function to illuminate picture
	illuminate(ncols, nrows, illum, pix, sx, sy, sz, xScale, yScale);

	// writes file is grayscale
	if (grayscale == true)
	{
		write_file_pgm(outfname, ncols, nrows, illum);
	}

	// writes file if colored
	if (grayscale == false)
	{
		write_file_ppm(outfname, ncols, nrows, minElev, maxElev, illum, pix);
	}

	return 0;
}

// gets file an output file name
bool get_file(string &infname, string &outfname, bool &grayscale)
{
	// prompts user for input file name
	cout << "Please input the name of the Elevation Map file:" << endl;
	cin >> infname;

	ifstream ifs(infname);
	
	// checks to make sure file opened correctly
	if (!ifs)
	{
		cerr << "Error in opening file '" << infname << "'." << endl;
		return false;
	}

	ifs.close();

	// calls function to get the basename of infname
	string basefname = get_basename(infname);

	int chooseGray;

	// prompts user to choose grayscale or colored
	cout << endl << "Would you like the image to be grayscale or colored?" << endl
		<< "1. Grayscale" << endl
		<< "2. Color" << endl;

	cin >> chooseGray;

	// sets bool grayscale depending on choice
	switch (chooseGray)
	{
	case 1:
		grayscale = true;
		break;

	case 2:
		grayscale = false;
		break;

	default:
		cerr << "Input not recognized." << endl << endl;
		return false;
	}

	int chooseName;

	// prompts user to input an output fname or have it be generated
	cout << endl << "Would you like to choose the name of the output file or have it automatically generated?" << endl
		<< "1. Input name" << endl
		<< "2. Generate name" << endl;

	cin >> chooseName;

	switch (chooseName)
	{
	case 1:
		cout << "Please input the file name: ";
		cin >> outfname;
		break;

	case 2:
		if (grayscale == true)
		{
			outfname = basefname + "_image.pgm";
			cout << endl << "The file has been named '" << outfname << endl << endl;
		}

		else
		{
			outfname = basefname + "_image.ppm";
			cout << endl << "The file has been named '" << outfname << endl << endl;
		}
		break;

	default:
		cerr << endl << "Input not recognized." << endl << endl;
		return false;
	}

	return true;
}

string get_basename(string fname)
{
	// gets basename from infname
	int i = (fname.length() - 1);

	while (fname[i] != '.' && i >= 0)
	{
		i--;
	}

	return(fname.substr(0, i));
}

// reads file into variables and data into an array
void read_file(string &infname, string &type, int &ncols, int &nrows, float &minElev, float &maxElev,
	float &xScale, float &yScale, float pix[])
{
	ifstream ifs(infname);

	ifs >> type >> ncols >> nrows >> minElev >> maxElev >> xScale >> yScale;

	for (int i = 0; i < (ncols * nrows); i++)
	{
		ifs >> pix[i];
	}

	ifs.close();
}

// function to illuminate picture
void illuminate(int ncols, int nrows, float illum[], float pix[], float sx, float sy, float sz,
	float xScale, float yScale)
{
	float h0, hx, hy;
	float a, b, c;

	for (int i = 0; i < ncols; i++)
	{
		for (int j = 0; j < nrows; j++)
		{
			// sets h0 to current pixel
			h0 = pix[j + i*nrows];

			// if the last row/col sets the illumination to the pixel above/previous
			if (j == (nrows - 1))
			{
				illum[j + i*nrows] = illum[j + i*nrows - 1];
			}

			else if (i == (ncols - 1))
			{
				illum[j + i*nrows] = illum[j + i*nrows - ncols];
			}

			else
			{
				// sets hx and hy to pixels to the right and below h0
				hx = pix[j + i*nrows + 1];
				hy = pix[j + i*nrows + ncols];

				// calls function to find the surface norma;
				surf_norm(h0, hx, hy, xScale, yScale, a, b, c);

				// calculates the illumination
				illum[j + i*nrows] = 255 * (dotp(a, b, c, sx, sy, sz) * 0.9 + 0.1);
			}
		}
	}
}

// function to colorize picture
void colorize(int ncols, int nrows, float minElev, float maxElev, float pix[], float illum[], float red[], float green[], float blue[])
{
	// opens colormap
	ifstream ifs("colormap_mountains.txt");

	// checks to see if colormap opened
	if (!ifs)
	{
		cerr << "Error in opening colormap_mountains.txt" << endl;
	}

	// reads colormap into an array
	int colormap[256 * 3];

	for (int i = 0; i < (256 * 3); i++)
	{
		ifs >> colormap[i];
	}

	ifs.close(); 

	int elev;

	for (int i = 0; i < ncols; i++)
	{
		for (int j = 0; j < nrows; j++)
		{
			// calls function to scale elevation 
			elev = 255 * scale(pix[j + i*nrows], minElev, maxElev);

			// gets colors for red green and blue
			red[j + i*nrows] = colormap[3 * elev];
			green[j + i*nrows] = colormap[3 * elev + 1];
			blue[j + i*nrows] = colormap[3 * elev + 2];

			// calculates color * illumination
			red[j + i*nrows] = (illum[j + i*nrows]/255) * red[j + i*nrows];
			green[j + i*nrows] = (illum[j + i*nrows] / 255) * green[j + i*nrows];
			blue[j + i*nrows] = (illum[j + i*nrows] / 255) * blue[j + i*nrows];
		}
	}
}

// function to write to a pgm file
void write_file_pgm(string outfname, int ncols, int nrows, float illum[])
{
	ofstream ofs(outfname);

	ofs << "P2 " << ncols << " " << nrows << " " << 255 << endl;

	for (int i = 0; i < ncols; i++)
	{
		for (int j = 0; j < nrows; j++)
		{
			ofs << static_cast<int>(illum[j + i*nrows]) << " ";
		}
		ofs << endl;
	}

	ofs.close();

	// outputs header information
	cout << endl << "The data has been written to " << outfname << endl
		<< "The header information for " << outfname << " is:" << endl
		<< "P2 " << ncols << " " << nrows << " " << 255 << endl;
}
 // writes to a ppm file
void write_file_ppm(string outfname, int ncols, int nrows, float minElev, float maxElev, float illum[], float pix[])
{
	float red[SIZE] = { 0 };
	float green[SIZE] = { 0 };
	float blue[SIZE] = { 0 };

	colorize(ncols, nrows, minElev, maxElev, pix, illum, red, green, blue);

	ofstream ofs(outfname);

	ofs << "P3 " << ncols << " " << nrows << " " << 255 << endl;

	for (int i = 0; i < ncols; i++)
	{
		for (int j = 0; j < nrows; j++)
		{
			ofs << static_cast<int>(red[j + i*nrows]) << " " << static_cast<int>(green[j + i*nrows]) 
				<< " " << static_cast<int>(blue[j + i*nrows]) << " ";
		}
		ofs << endl;
	}

	ofs.close();

	// outputs header information
	cout << endl << "The data has been written to " << outfname << endl
		<< "The header information for " << outfname << " is:" << endl
		<< "P3 " << ncols << " " << nrows << " " << 255 << endl;
}

// function to normalize
void norm(float x, float y, float z, float &xnorm, float &ynorm, float &znorm)
{
	float magnitude = mag(x, y, z);

	xnorm = x / magnitude;
	ynorm = y / magnitude;
	znorm = z / magnitude;

}

// function to calculate surface normal
void surf_norm(float h0, float hx, float hy, float xScale, float yScale, float &a, float &b, float &c)
{
	float deltazx = (hx - h0);
	float deltazy = (hy - h0);

	float axnorm, aynorm, aznorm, bxnorm, bynorm, bznorm;
	norm(xScale, 0, deltazx, axnorm, aynorm, aznorm);
	norm(0, yScale, deltazy, bxnorm, bynorm, bznorm);

	crossp(axnorm, aynorm, aznorm, bxnorm, bynorm, bznorm, a, b, c);
}

// function to scale value
float scale(float pixel, float min, float max)
{
	float result;

	result = ((pixel - min) / (max - min));

	return result;
}

// converts az el to x y z
void ae2xyz(float az, float el, float &x, float &y, float &z)
{
	float xnot, ynot, znot;
	xnot = cos(el) * cos(az); 
	ynot = cos(el) * sin(az); 
	znot = sin(el); 

	norm(xnot, ynot, znot, x, y, z);
}

// magnitude function
float mag(float x, float y, float z)
{
	float result;

	result = sqrt(x*x + y*y + z*z);

	return result;
}

// dot product function
float dotp(float x1, float y1, float z1, float x2, float y2, float z2)
{
	float result;

	result = x1 * x2 + y1 * y2 + z1 * z2;

	return result;
}

// cross product function
void crossp(float x1, float y1, float z1, float x2, float y2, float z2, float &cx, float &cy, float &cz)
{
	cx = y1*z2 - z1*y2;
	cy = z1*x2 - x1*z2;
	cz = x1*y2 - y1*x2;
}
