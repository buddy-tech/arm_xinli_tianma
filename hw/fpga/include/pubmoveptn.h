/*
 * pubmoveptn.h
 *
 *  Created on: 2018-7-24
 *      Author: tys
 */

#ifndef PUBMOVEPTN_H_
#define PUBMOVEPTN_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BMP_HEAD_SIZE (54)
#define PICTURE_PKT_SIZE (320*1024)

int move_ptn_getPostionByName(char *pBmpName);

int move_ptn_download(char *pBmpName,int width,int height,int bAuto);

#ifdef __cplusplus
}
#endif
#endif /* PUBMOVEPTN_H_ */
