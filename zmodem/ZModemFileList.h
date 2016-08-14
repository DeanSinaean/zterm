#ifndef FILELIST
#define FILELIST

#include <string.h>
class Filelist 
{
	public:
		Filelist(){
			count = 0;
		};
		~Filelist();
		int GetSize() {
			return count;
		}
		void Add(char *filename) {
			files[count]=strdup(filename);
			count++;
		}
		char * GetAt(int i) {
			return files[i];
		}

	public:

	private:
		int count;
		char * files[500];

};
#endif
