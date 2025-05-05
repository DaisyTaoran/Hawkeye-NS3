#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from sklearn import svm
from sklearn.svm import SVC
from sklearn.preprocessing import MinMaxScaler
from sklearn.model_selection import GridSearchCV
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import (confusion_matrix, classification_report)

#数据读取
filename = 'train_6f.csv'
data_train = pd.read_csv('train_6f.csv')
data_test = pd.read_csv('test_6f.csv')
X_train = data_train.drop(['Label'], axis=1)  
X_test = data_test.drop(['Label'], axis=1)  
y_train = data_train['Label']
y_test = data_test['Label']
print "数据读取  完成" 
       
#标准化StandardScaler()  归一化
scaler = MinMaxScaler(feature_range=(0.0,1.0))
X_train_scaled = scaler.fit_transform(X_train.astype(float))
X_test_scaled = scaler.fit_transform(X_test.astype(float))
print "数据标准化  完成"

#SVM模型训练: linear, poly, sigmoid, rbf
'''网格搜索，最优参数: {'kernel': 'rbf', 'C': 1, 'gamma': 'scale'}
param_grid = {
    'C': [0.1, 1], 
    'gamma': ['scale', 'auto', 0.01, 0.1],
    'kernel': ['rbf', 'linear']
    'max_iter': [1000000]
}
grid = GridSearchCV(svm.SVC(), param_grid, refit=True, cv=5, scoring='f1')
grid.fit(X_train_scaled, y_train) 
print "最优参数:", grid.best_params_
model = grid.best_estimator_
'''
model = SVC(kernel='rbf',gamma='scale', C=10, class_weight={0: 10, 1: 1})
model.fit(X_train_scaled, y_train)
print "模型训练  完成"
y_pred = model.predict(X_test_scaled)
print "模型测试  完成"

# 计算TP TN FP FN
TP = ((y_test == 1) & (y_pred == 1)).sum()  
TN = ((y_test == 0) & (y_pred == 0)).sum()  
FP = ((y_test == 0) & (y_pred == 1)).sum()  
FN = ((y_test == 1) & (y_pred == 0)).sum() 
total = TP + TN + FP + FN
CR = 1.0 *(TP + TN) / total if total != 0 else 0.0  
FNR = 1.0 * FN / (TP + FN) if (TP + FN) != 0 else 0.0 
FPR = 1.0 * FP / (FP + TN) if (FP + TN) != 0 else 0.0 
DR = 1.0 * TP / (TP + FN) if (TP + FN) != 0 else 0.0 
print "---- 6f 基础评价指标 ----"
print " CR: %.6f" % CR
print "FNR: %.6f" % FNR
print "FPR: %.6f" % FPR 
print " DR: %.6f" % DR
print "总支持向量数量:", len(model.support_vectors_)

# 绘制混淆矩阵
cm = confusion_matrix(y_test, y_pred)
plt.figure(figsize=(6, 4))
sns.heatmap(cm, annot=True, fmt='d', cmap='Blues', 
            xticklabels=["Normal", "Attack"], 
            yticklabels=["Normal", "Attack"])
plt.xlabel("Predicted Value")
plt.ylabel('True Value')
plt.title("Confusion Matrix")

output_file = filename.replace('.csv', '.png')
output_file = output_file.replace('csvdata', 'Confusion_Matrix')
plt.savefig(output_file, dpi=1000)
print "混淆矩阵已保存至：", output_file
plt.show()


