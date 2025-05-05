#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import os
import glob
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.ensemble import RandomForestClassifier

def get_feature_importance(filename='Feature_Importance.png'): # 输出并保存特征重要性评分
	# 训练随机森林
	model = RandomForestClassifier(n_estimators=100)
	rf = model.fit(X_train, y_train)
	print "RF Accuracy:", model.score(X_test, y_test)
	# 评估特征重要性
	feature_importances = pd.Series(rf.feature_importances_, index=X_train.columns)
	top_features = feature_importances.nlargest(15)
	# 绘制图表
	plt.figure()  
	top_features.plot(kind='barh', title='Feature Importance')
	plt.tight_layout() 
	# 保存图片
	plt.savefig(filename, dpi=1000)
	print "特征重要性评分已保存至：", filename
	plt.show()

def read_all_file(path):  # 读取文件夹 path 中所有txt文件的数据
	all_files = glob.glob(os.path.join(path, '*.txt'))
	datas = []
	for file in all_files:
		one = pd.read_csv(file, names=['src_ip', 'src_port', 'min_seq', 'max_seq', 'pkts_ack', 'pkts_fwd', 'bytes_fwd', 'bytes_bwd', 'enqdepth', 'pkts_pfcPause', 'flow_weight', 'node_weight', 'duration'])
		datas.append(one)
	combined_data = pd.concat(datas, ignore_index=True)
	return combined_data

def read_one_file(path):
	one = pd.read_csv(path, names=['src_ip', 'src_port', 'min_seq', 'max_seq', 'pkts_ack', 'pkts_fwd', 'bytes_fwd', 'bytes_bwd', 'enqdepth', 'pkts_pfcPause', 'flow_weight', 'node_weight', 'duration'])
	return one

def data_add_label(data): # 特征工程 + 打标签 + 去除不能用于训练的特征 + 清洗空数据
	data_c = data.copy()
	data_c = data_c[data_c['duration'] != 0]
	data_c['bps_fwd'] = data_c['bytes_fwd'] / data_c['duration']
	data_c['bps_bwd'] = data_c['bytes_bwd'] / data_c['duration']
	data_c['pps_fwd'] = data_c['pkts_fwd'] / data_c['duration']
	data_c['pps_ack'] = data_c['pkts_ack'] / data_c['duration']
	attack_ips = {'11.0.5.1', '11.0.6.1', '11.0.7.1', '11.0.8.1'}
	data_c['Label'] = data_c['src_ip'].apply(lambda ip: 1 if ip in attack_ips else 0)
	data_c = data_c.dropna().drop_duplicates()#, 'duration', 'pkts_ack'
	data_c = data_c.drop(data[['src_ip', 'src_port']], axis=1)
	data = data_c.copy()
	return data
	
def data_select_feature(data): # 选出9个特征'node_weight'
	selected_data = data.drop(data[['pkts_pfcPause', 'flow_weight','max_seq', 'min_seq', 'pkts_fwd', 'pps_fwd', 'bps_bwd', 'pps_ack', 'pkts_ack', ]], axis=1) 
	return selected_data


data_train = read_all_file('train')
data_test = read_all_file('test')
#data_test = read_one_file('test/teleflowdata_6.txt')

data_train = data_add_label(data_train)
data_test = data_add_label(data_test)

data_train = data_select_feature(data_train)
data_test = data_select_feature(data_test)

X_train = data_train.drop(data_train[['Label']], axis=1)
y_train = data_train['Label']

X_test = data_test.drop(data_test[['Label']], axis=1)
y_test = data_test['Label']

# 检查数据分布,并展示特征重要性评分
print 'Label distribute in train' 
print pd.Series(y_train).value_counts()
get_feature_importance()

# 保存为csv文件用于SVM训练和测试
df_train = pd.DataFrame(data_train)
df_test = pd.DataFrame(data_test)
filename_train = 'train_6f.csv'
filename_test = 'test_6f.csv'
df_train.to_csv(filename_train, index=False, encoding="utf-8")
df_test.to_csv(filename_test, index=False, encoding="utf-8")
print "筛选后的数据已保存至：", filename_train, ", ", filename_test








