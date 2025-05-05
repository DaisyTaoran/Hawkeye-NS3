#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import os
import time
import pickle
import psutil
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

from sklearn import svm
from sklearn.svm import SVC
from sklearn.preprocessing import MinMaxScaler
from sklearn.model_selection import GridSearchCV
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import (confusion_matrix, classification_report)

def measure_time(func):
    """测量函数执行时间"""
    def wrapper(*args, **kwargs):
        start_time = time.time()
        result = func(*args, **kwargs)
        end_time = time.time()
        return result, end_time - start_time
    return wrapper

def measure_memory():
    """获取当前进程内存占用（MB）"""
    process = psutil.Process(os.getpid())
    return process.memory_info().rss / 1024**2

@measure_time
def train_svm(X_train, y_train):
    model = SVC(kernel='rbf',gamma='scale', C=10, class_weight={0: 10, 1: 1})
    model.fit(X_train, y_train)
    return model

@measure_time
def svm_infer(model, X_test):
    return model.predict(X_test)
    

def get_model_size(model, filename):
    with open(filename, 'wb') as f:
        pickle.dump(model, f)
    return os.path.getsize(filename) / 1024  # KB

data_train = pd.read_csv('train_6f.csv')
data_test = pd.read_csv('test_6f.csv')
X_train = data_train.drop(['Label'], axis=1)  
X_test = data_test.drop(['Label'], axis=1)  
y_train = data_train['Label']
y_test = data_test['Label']
print "数据读取  完成" 

scaler = MinMaxScaler(feature_range=(0.0,1.0))
X_train_scaled = scaler.fit_transform(X_train.astype(float))
X_test_scaled = scaler.fit_transform(X_test.astype(float))
print "数据归一化  完成"

svm_model, svm_train_time = train_svm(X_train, y_train)
svm_train_memory = measure_memory()
print "SVM 训练时间 = ", svm_train_time
print "SVM 训练内存 = ", svm_train_memory

_, svm_infer_time = svm_infer(svm_model, X_test)
print "SVM 推理时间 = ", svm_train_time

svm_size = get_model_size(svm_model, 'svm_model.pkl')
print "SVM 模型大小 = ", svm_size



