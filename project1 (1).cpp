#pragma comment(linker, "/STACK:16777216")
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>

using namespace std;

#include "project2.h"

bool get_file(string &infname, string &outfname);
void read_EGM(string infname, EGM &file);
void illuminate(EGM egm_file, vector<double> &illum, float sx, float sy, float sz);
void colorize(EGM egm_file, vector<double> illum, vector<ColorPix> &color);
void write_PPM(string outfname, EGM egm_file, vector<double> illum);

void norm(float x, float y, float z, float &xnorm, float &ynorm, float &znorm);
void surf_norm(float h0, float hx, float hy, float xScale, float yScale, float &a, float &b, float &c);
float scale(float pixel, float min, float max);
void ae2xyz(float az, float el, float &x, float &y, float &z);
float mag(float x, float y, float z);
float dotp(float x1, float y1, float z1, float x2, float y2, float z2);
void crossp(float x1, float y1, float z1, float x2, float y2, float z2, float &cx, float &cy, float &cz);

int main(void)
{
	string infname, outfname;
	EGM egm_file;

	get_file(infname, outfname);

	if (!get_file)
		return 1;

	read_EGM(infname, egm_file);

	// outputs header information
	cout << "The header information for " << infname << " is:" << endl
		<< "E2 " << egm_file.ncols << " " << egm_file.nrows << " " << egm_file.minElev << " " << egm_file.maxElev << " "
		<< egm_file.xScale << " " << egm_file.yScale << endl << endl;

	float az, el, sx, sy, sz;

	// prompts user to input sun's positiion
	cout << "Please input the azimuth and elevation of the sun's position in radians:" << endl;
	cin >> az >> el;

	cout << "Working, please wait..." << endl;

	// calls function to convert azimuth and elevation to x y z
	ae2xyz(az, el, sx, sy, sz);

	vector<double> illum;

	illum.resize(egm_file.ncols * egm_file.nrows);

	illuminate(egm_file, illum, sx, sy, sz);

	write_PPM(outfname, egm_file, illum);

	return 0;
}

bool get_file(string &infname, string &outfname)
// prompts user for input file name
{
	cout << "Please input the name of the EGM file:" << endl;
	cin >> infname;

	ifstream ifs(infname);

	// checks to make sure file opened correctly
	if (!ifs)
	{
		cerr << "Error in opening file '" << infname << "'." << endl;
		return false;
	}

	ifs.close();

	cout << "Please input the output file name:" << endl;
	cin >> outfname;

	return true;
}

void read_EGM(string infname, EGM &file)
{
	char type[8];
	int header[6];
	vector<int> pix_data;

	ifstream ifs(infname, ios::binary);
	ifs.read(type, 8);
	ifs.read(reinterpret_cast<char *>(header), 6 * sizeof(int));

	pix_data.resize(header[0] * header[1]);

	ifs.read(reinterpret_cast<char *>(&pix_data[0]), header[0] * header[1] * sizeof(int));

	ifs.close();

	file.ncols = header[0];
	file.nrows = header[1];
	file.minElev = header[2];
	file.maxElev = header[3];
	file.xScale = header[4];
	file.yScale = header[5];
	file.pix.resize(file.ncols * file.nrows);

	cout << "Working, please wait..." << endl;

	for (int i = 0; i < file.ncols * file.nrows; i++)
	{
		file.pix[i] = pix_data[i];
	}
}

// function to illuminate picture
void illuminate(EGM egm_file, vector<double> &illum, float sx, float sy, float sz)
{
	float h0, hx, hy;
	float a, b, c;

	for (int i = 0; i < egm_file.ncols; i++)
	{
		for (int j = 0; j < egm_file.nrows; j++)
		{
			// sets h0 to current pixel
			h0 = egm_file.pix[j + i*egm_file.nrows];

			// if the last row/col sets the illumination to the pixel above/previous
			if (j == (egm_file.nrows - 1))
			{
				cout << "here" << endl;
				illum[j + i*egm_file.nrows] = illum[j + i*egm_file.nrows - 1];
			}

			else if (i == (egm_file.ncols - 1))
			{
				cout << "here2" << endl;
				illum[j + i*egm_file.nrows] = illum[j + i*egm_file.nrows - egm_file.ncols];
			}

			else
			{
				// sets hx and hy to pixels to the right and below h0
				hx = egm_file.pix[j + i*egm_file.nrows + 1];
				hy = egm_file.pix[j + i*egm_file.nrows + egm_file.ncols];

				// calls function to find the surface norma;
				surf_norm(h0, hx, hy, egm_file.xScale, egm_file.yScale, a, b, c);

				// calculates the illumination
				illum[j + i*egm_file.nrows] = 255 * (dotp(a, b, c, sx, sy, sz) * 0.9 + 0.1);

				if (i == static_cast<int>(egm_file.ncols*egm_file.nrows / 4))
					cout << "25% ";

				else if (i == static_cast<int>(egm_file.ncols*egm_file.nrows / 2))
					cout << "50% ";

				else if (i == static_cast<int>(egm_file.ncols*egm_file.nrows * 3/4))
					cout << "75% ";
			}
		}
	}
}

void colorize(EGM egm_file, vector<double> illum, vector<ColorPix> &color)
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

	for (int i = 0; i < egm_file.ncols; i++)
	{
		for (int j = 0; j < egm_file.nrows; j++)
		{
			// calls function to scale elevation 
			elev = 255 * scale(egm_file.pix[j + i*egm_file.nrows], egm_file.minElev, egm_file.maxElev);

			// gets colors for red green and blue
			color[j + i*egm_file.nrows].red = colormap[3 * elev];
			color[j + i*egm_file.nrows].green = colormap[3 * elev + 1];
			color[j + i*egm_file.nrows].blue = colormap[3 * elev + 2];

			// calculates color * illumination
			color[j + i*egm_file.nrows].red = (illum[j + i*egm_file.nrows] / 255) * color[j  + i*egm_file.nrows].red;
			color[j + i*egm_file.nrows].green = (illum[j + i*egm_file.nrows] / 255) * color[j + i*egm_file.nrows].green;
			color[j + i*egm_file.nrows].blue = (illum[j + i*egm_file.nrows] / 255) * color[j + i*egm_file.nrows].blue;
		}
	}
}

void write_PPM(string outfname, EGM egm_file, vector<double> illum)
{
	vector<ColorPix> color;
	color.resize(egm_file.ncols * egm_file.nrows);

	colorize(egm_file, illum, color);

	ofstream ofs(outfname);

	ofs << "P3 " <<egm_file.ncols << " " << egm_file.nrows << " " << 255 << endl;

	for (int i = 0; i < egm_file.ncols; i++)
	{
		for (int j = 0; j < egm_file.nrows; j++)
		{
			ofs << static_cast<int>(color[j + i*egm_file.nrows].red) << " " << static_cast<int>(color[j + i*egm_file.nrows].green)
				<< " " << static_cast<int>(color[j + i*egm_file.nrows].blue) << " ";
		}
		ofs << endl;
	}

	ofs.close();

	// outputs header information
	cout << endl << "The data has been written to " << outfname << endl
		<< "The header information for " << outfname << " is:" << endl
		<< "P3 " << egm_file.ncols << " " << egm_file.nrows << " " << 255 << endl;
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