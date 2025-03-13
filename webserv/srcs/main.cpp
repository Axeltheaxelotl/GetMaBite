/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smasse <smasse@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/05 15:26:22 by smasse            #+#    #+#             */
/*   Updated: 2025/03/12 17:27:52 by smasse           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include"webserv.hpp"

void get_files(const std::string &directory, std::vector<std::string> &files)
{
	DIR *dir;
	struct dirent *ent;
	if((dir = opendir(directory.c_str())) != NULL)
	{
		while((ent = readdir(dir)) != NULL)
		{
			if(ent->d_type == DT_REG)
			{
				files.push_back(directory + "/" + ent->d_name);
			}
		}
		closedir(dir);
	}
	else
	{
		std::cerr << "Could not open directory: " << directory << std::endl;
	}
}

int main(void)
{
	std::vector<std::string> good_files;
	std::vector<std::string> bad_files;
	get_files("./config/good", good_files);
	get_files("./config/bad", bad_files);
	for(std::vector<std::string>::iterator it = good_files.begin(); it != good_files.end(); ++it)
	{
		if(!parser(*it))
		{
			std::cout << "Mismatch: " << *it << " should be good but failed." << std::endl;
		}
	}
	for(std::vector<std::string>::iterator it = bad_files.begin(); it != bad_files.end(); ++it)
	{
		if(parser(*it))
		{
			std::cout << "Mismatch: " << *it << " should be bad but passed." << std::endl;
		}
	}
	return (0);
}
