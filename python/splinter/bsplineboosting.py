# This file is part of the SPLINTER library.
# Copyright (C) 2012 Bjarne Grimstad (bjarne.grimstad@gmail.com).
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

from . import splinter
from .bsplinebuilder import BSplineBuilder
import numpy as np


class BSplineBoosting:
    def __init__(self, loss: str='ls', learning_rate: float=0.1, n_estimators: int=100, subsample: float=1.0,
                 alpha: float=1.0):
        """
        Class for stochastic gradient boosting with B-spline learners
        :param loss: loss function, 'ls' for least squares loss function
        :param learning_rate: Learning rate applied to new estimators. Advised to be in the range (0, 0.1).
        :param n_estimators: Number of estimators. Inversely proportional to learning_rate, e.g. a high number of estimators is required if the learning rate is low.
        :param subsample: Amount of samples (drawn randomly) to use when fitting a new estimator. Must be a number in (0, 1].
        :param alpha: Regularization parameter for P-spline learners. Must be a strictly positive number.
        TODO: add init and warm_start as in sklearn.ensemble.GradientBoostingRegressor
        """
        # super(Function, self).__init__()
        self._loss = ls  # Function pointer

        if not 0 < learning_rate < 1:
            raise ValueError("'learning_rate' must be a number in (0, 1)")
        self._learning_rate = learning_rate

        if not n_estimators > 0:
            raise ValueError("'n_estimators' must be a strictly positive number")
        self._n_estimators = n_estimators

        if not 0 < subsample <= 1:
            raise ValueError("'subsample' must be a number in (0, 1]")
        self._subsample = subsample

        if not alpha > 0:
            raise ValueError("'alpha' must be a strictly positive number")
        self._alpha = alpha

        self._learner = 'pspline'
        self._estimators = [None] * self._n_estimators
        self._oob_improvement = np.array((self._n_estimators,))
        self._train_score = np.array((self._n_estimators,))

    def fit(self, x: np.ndarray, y: np.ndarray):
        """
        Fit to data (x, y)
        """
        n = x.shape[0]

        # Initialize first base learner
        self._estimators[0] = Const(np.mean(y) / self._learning_rate)

        for i in range(1, self._n_estimators):
            u_hat = y - self.eval(x)

            learners = [None] * n
            goodness = np.zeros((n,))
            ss_tot = np.sum(np.apply_along_axis(np.square, 0, u_hat - np.mean(u_hat)))

            for j in range(n):
                learners[j] = BSplineBuilder(x, u_hat,
                                             smoothing=splinter.BSplineBuilder.Smoothing.PSPLINE,
                                             alpha=self._alpha,
                                             knot_spacing=splinter.BSplineBuilder.KnotSpacing.EXPERIMENTAL,
                                             num_basis_functions=20).build()
                u_hat_est = learners[j].eval(x)
                ss_res = np.sum(np.apply_along_axis(np.square, 0, u_hat - u_hat_est))
                goodness[j] = 1 - (ss_res / ss_tot)

            best_learner = 0
            for j in range(1, n):
                if goodness[j] > goodness[best_learner]:
                    best_learner = j

            self._estimators[i] = learners[best_learner]

    def eval(self, x):
        """
        Evaluate ensemble at x
        """
        y = np.zeros(x.shape)
        for i in range(self._n_estimators):
            if self._estimators[i] is not None:
                y += self._learning_rate * np.array(self._estimators[i].eval(x))
            else:
                break
        return y

    def predict(self, x):
        """
        Predict value at x
        """
        return self.eval(x)


class Const:
    def __init__(self, constant):
        self._const = constant

    def eval(self, x):
        return self._const * np.ones(x.shape)


def ls(x):
    return sse(x, np.zeros(x.shape))


def sse(x: np.ndarray, y: np.ndarray):
    assert(x.shape == y.shape)
    assert(x.ndim == 1)
    return np.sum(np.apply_along_axis(np.square, 0, x - y))


def mse(x: np.ndarray, y: np.ndarray):
    assert(x.shape == y.shape)
    assert(x.ndim == 1)
    return sse(x, y) / x.shape[0]
