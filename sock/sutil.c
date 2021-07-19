#include "sutil.h"
#include "log.h"


int sock_fd_init(int type, u16_t port)
{
    int fd = 0;
    int ret = 0;
    struct sockaddr_in addr;
    
    SOCK_LOG("type = %s, port = %u\r\n", SOCK_TCP==type?"TCP":"UDP", port);
    
    fd = socket(AF_INET, SOCK_STREAM, 0); 
    if(-1 == fd)
    {
        SOCK_ERR("socket fail!\n");
        goto err_socket;
    }
    
    /* for rebinding */
    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if(-1 == ret )
    {
        SOCK_ERR("bind fail!\n");
        goto err_bind;
    }

    ret = listen(fd, 3);
    if(-1 == ret)
    {
        SOCK_ERR("listen fail!\n");
        goto err_listen; 
    }
    SOCK_OK("tcp socket(%d:%u) init success\r\n", fd, port);
    
    return fd;
err_listen:
err_bind:
err_socket:
    return -1;
}

