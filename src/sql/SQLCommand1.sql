----------------------------
-- 终端形式下非登录执行SQL语句
----------------------------

# 基本格式
mysql -u 用户名 -p -e "SQL语句"

# 查看所有数据库
mysql -u root -p -e "SHOW DATABASES;"

# 执行多条语句（用分号隔开）
mysql -u root -p -e "USE test_db; SELECT * FROM users LIMIT 5;"

# 管道执行
echo "SELECT VERSION();" | mysql -u root -p

----------------------------
-- 终端形式下非登录执行SQL文件
----------------------------

# script.sql(文件内容：SELECT * FROM test_db.users);
mysql -u root -p < script.sql

--------------------------
-- 终端形式下登录执行SQL语句
--------------------------

# 将用户名替换，执行后输入该用户密码，进入mysql>提示符界面
mysql -u 用户名 -p

-------------
-- 创建用户
-------------

# 基本格式
CREATE USER IF NOT EXISTS '用户名'@'主机地址' IDENTIFIED BY '密码';

# localhost指只能本机登录
CREATE USER IF NOT EXISTS 'test_user'@'localhost' IDENTIFIED BY '123456';

# %指可以远程登录
CREATE USER IF NOT EXISTS 'remote_user'@'%' IDENTIFIED BY 'mypass';

-------------
-- 用户授权
-------------

# 授予该用户数据库test_db所有权
GRANT ALL PRIVILEGES ON test_db.* TO 'test_user'@'localhost';

# 授予该用户所有数据库的查询和插入权限
GRANT SELECT, INSERT ON *.* TO 'remote_user'@'%';

# 刷新该用户权限，使授权立即生效(MySQL8.0中GRANT会自动刷新)
FLUSH PRIVILEGES;

-------------
-- 查看用户
-------------

# 查看所有用户的基本信息(用户名和主机地址)
SELECT User, Host FROM mysql.user;

# 查看用户的详细信息(包含权限、密码策略等)
SELECT * FROM mysql.user;

# 查看当前登录的用户(方式1)
SELECT CURRENT_USER();

# 查看当前登录的用户(方式2)
SELECT USER();

# 查看当前用户的权限
SHOW GRANTS;

# 查看指定用户的权限
SHOW GRANTS FOR '用户名'@'主机地址';

-------------
-- 删除用户
-------------

# 删除指定用户
DROP USER '用户名'@'主机地址';

-----------------------
-- 锁定/解锁指定用户权限
-----------------------

# 锁定用户
ALTER USER 'test_user'@'localhost' ACCOUNT LOCK;

# 解锁用户
ALTER USER 'test_user'@'localhost' ACCOUNT UNLOCK;

----------
-- 切换用户
----------

# 已登录情况下切换用户

# 1.退出当前MySQL会话
exit;

# 2.在终端中使用新用户登录
mysql -u 新用户名 -p

# 未登录情况下切换用户

# 以test_user用户登录，并指定登录的数据库为test_db
mysql -u test_user -p -D test_db

# 以test_user用户登录，并指定主机地址（如果是远程服务器）
mysql -u test_user -h 192.168.1.100 -p