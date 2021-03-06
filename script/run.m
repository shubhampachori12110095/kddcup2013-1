function [X,y,X2,y1,y2,model] = run(params)
FEATURE_EXTRACTOR_EXE_PATH = '..\FeatureExtractor\x64\Release\FeatureExtractor.exe';
DATA_DIRECTORY = '..\data';

%% Experimental settings..
algorithm = params.algorithm;                       % 0: logistic regression, 1: decision tree, 2: kernel SVM, 3: KNN
preprocessing_method = params.preprocessing_method; % Preprocessing method : 'Z' or 'minmax'
postprocessing_critical = params.postprocessing_critical; % Whether using critical feature as postprocessing or not
feature_list = params.feature_list; % Feature ids that will be used for training and prediction
be = params.be; % basis expansion?
fprintf('algorithm : %d\n', algorithm);
fprintf('preprocessing : %s\n', preprocessing_method);
fprintf('postprocessing? : %d\n', postprocessing_critical);
fprintf('basis_exp : %d\n', be);

%% Extract features
% delete('..\data\*.predict.*');
% tic;
% fprintf('Extracting features... ');
% system(sprintf('%s %s', FEATURE_EXTRACTOR_EXE_PATH, DATA_DIRECTORY));
% fprintf('%f s\n', toc);

%% Load feature data
fprintf('feature list : ');
fprintf('%d ', feature_list);
fprintf('\n');

train_data = loadFeature('Train', feature_list);
valid_data = loadFeature('Valid', feature_list);
R = randperm(size(train_data, 1));
train_data = train_data(R, :);
train_data = sortrows(train_data, 1);

X = train_data(:, 3:(size(train_data,2)-1));
y = train_data(:, end);
X2 = valid_data(:, 3:(size(valid_data,2)-1));

X  = preprocessing(X, preprocessing_method);
X2 = preprocessing(X2, preprocessing_method);

if be
    X = basis_expansion(X);
    X2 = basis_expansion(X2);
end

%% Learn predict and output
[y1, y2, model] = learnAndPredict(X, y, X2, algorithm);
tic;
fprintf('Training Output... ');
fp_predict = fopen(sprintf('%s\\Train.predict.csv', DATA_DIRECTORY), 'w');
fprintf(fp_predict, 'author_id,paper_id,features\n');
dlmwrite(sprintf('%s\\Train.predict.csv', DATA_DIRECTORY), [train_data(:,1:(size(train_data,2)-1)) y1], 'delimiter', ',', 'precision', 10, '-append');
fprintf('%f s\n', toc);
fclose(fp_predict);

tic;
fprintf('Valid Output... ');
fp_predict = fopen(sprintf('%s\\Valid.predict.csv', DATA_DIRECTORY), 'w');
fprintf(fp_predict, 'author_id,paper_id,features\n');
dlmwrite(sprintf('%s\\Valid.predict.csv', DATA_DIRECTORY), [valid_data(:,1:(size(valid_data,2)-1)) y2], 'delimiter', ',', 'precision', 10, '-append');
fprintf('%f s\n', toc);
fclose(fp_predict);

%% Unfold result and calculate MAP score
system(sprintf('python ..\\script\\result.py ..\\data\\Train.predict.csv %d', postprocessing_critical));
system(sprintf('python ..\\script\\result.py ..\\data\\Valid.predict.csv %d', postprocessing_critical));
system('python ..\script\eval.py ..\data\Train.predict.result.csv ..\data\Train.csv');
system('python ..\script\eval.py ..\data\Valid.predict.result.csv ..\data\ValidSolution.csv');
system('python ..\script\error.py ..\data\Train.predict.csv ..\data\Train.csv');
system('python ..\script\error.py ..\data\Valid.predict.csv ..\data\ValidSolution.csv');
end