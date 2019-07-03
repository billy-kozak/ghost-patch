/******************************************************************************
* Copyright (C) 2019  Billy Kozak                                             *
*                                                                             *
* This file is part of the gorilla-patch program                              *
*                                                                             *
* This program is free software: you can redistribute it and/or modify        *
* it under the terms of the GNU Lesser General Public License as published by *
* the Free Software Foundation, either version 3 of the License, or           *
* (at your option) any later version.                                         *
*                                                                             *
* This program is distributed in the hope that it will be useful,             *
* but WITHOUT ANY WARRANTY; without even the implied warranty of              *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
* GNU Lesser General Public License for more details.                         *
*                                                                             *
* You should have received a copy of the GNU Lesser General Public License    *
* along with this program.  If not, see <http://www.gnu.org/licenses/>.       *
******************************************************************************/
#ifndef MISC_MACROS_H
#define MISC_MACROS_H
/******************************************************************************
*                                   MACROS                                    *
******************************************************************************/
#define NUM_ARGS(type, ...)  (sizeof((type[]){__VA_ARGS__})/sizeof(type))
#define ARR_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))
/*****************************************************************************/
#endif /* MISC_MACROS_H */